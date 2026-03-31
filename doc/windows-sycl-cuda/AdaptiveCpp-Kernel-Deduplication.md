# AdaptiveCpp Kernel Deduplication Analysis

**Date**: 2026-03-29
**Status**: Confirmed — tested with rebuilt clang without stable naming patches
**Upstream issue**: [AdaptiveCpp/AdaptiveCpp#2024](https://github.com/AdaptiveCpp/AdaptiveCpp/issues/2024)

## Summary

When the same SYCL kernel template is instantiated in multiple translation units (common with header-included template code like OpenFOAM's NoRepository pattern), AdaptiveCpp JIT-compiles each TU's copy independently. There is no deduplication at compile time, link time, or runtime. This affects all platforms (Linux and Windows) equally.

## Root Cause

Each TU gets a **random** HCF object ID at compile time (`TargetSeparationPass.cpp:509`):

```cpp
std::size_t HcfObjectId = generateRandomNumber<std::size_t>();
```

The runtime kernel cache key (`kernel_configuration::id_type`) includes `hcf_object_id` as a base parameter (`kernel_configuration.hpp:38`). Since each TU has a unique random HCF ID, kernels from different TUs always produce different cache keys — even when the kernel name and device IR are byte-identical.

## Evidence

### Test: 2 TUs sharing the same kernel template

```
test_kernel_dedup_tu1.cpp  →  HCF object 5063389039308609046 (8 kernels)
test_kernel_dedup_tu2.cpp  →  HCF object 14183736377551346708 (8 kernels, SAME names)
```

With stable naming (hash-based): both TUs register 8 kernels under **identical** names (`__acpp_kernel_3ba2a7ec29042286`, etc.). But the runtime still JITs each separately because the HCF object IDs differ.

### Cache key composition (all backends: CUDA, HIP, OMP)

From `cuda_queue.cpp:549-558` (identical pattern in `omp_queue.cpp`, `hip_queue.cpp`):

```cpp
kernel_configuration config;
config.append_base_configuration(kernel_base_config_parameter::backend_id, backend_id::cuda);
config.append_base_configuration(kernel_base_config_parameter::compilation_flow, ...);
config.append_base_configuration(kernel_base_config_parameter::hcf_object_id, hcf_object);  // ← unique per TU
config.append_base_configuration(kernel_base_config_parameter::target_arch, selected_target);
```

The `hcf_object_id` makes the cache key unique per TU, so `get_or_construct_jit_code_object()` never gets a cache hit for the same kernel from a different TU.

### Persistent cache also affected

`persistent_cache_lookup(id_of_binary, ...)` uses the same HCF-object-dependent ID. Disk-cached JIT results from one TU's kernel are not reused for the same kernel from another TU.

### Scale in SPUMA

- **675 .o files**, 333 contain SYCL kernel code (49%)
- Each DLL that instantiates `_backendFor<Field<scalar>::op>` gets its own HCF blob with its own random ID
- simpleFoam loads ~20 DLLs → a common kernel like `Field<scalar>::operator+=` may be JIT'd up to 20 times
- First-iteration JIT time: ~6.5s currently. Could be significantly lower with deduplication.

## Affected Platforms

**All platforms equally.** On Linux (Itanium ABI), kernel names are deterministic across TUs, but the HCF object ID is still random per TU. The JIT duplication is identical.

## MSVC-Specific: Stable Kernel Naming

On MSVC ABI, lambda mangling includes TU-specific components (`?A0x<hash>`, `<lambda_N>`). AdaptiveCpp's upstream `replaceInvalidMSABICharsInSymbolName` (under `#ifdef _MSC_VER`) sanitizes these for name matching within a TU. Our stable naming patch (FNV hash canonicalization) makes names match ACROSS TUs — but since the HCF object ID prevents cross-TU cache hits anyway, this provides no JIT dedup benefit.

The stable naming patch has **no correctness or performance impact** given the current runtime architecture. It only affects debug output readability (consistent kernel names across TUs).

## Potential Fix (for upstream issue)

Replace the random HCF object ID with a **content hash** of the device IR module:

```cpp
// Instead of:
std::size_t HcfObjectId = generateRandomNumber<std::size_t>();

// Use content hash:
std::string ModuleContent;
llvm::raw_string_ostream OS(ModuleContent);
llvm::WriteBitcodeToFile(*DeviceModule, OS);
std::size_t HcfObjectId = std::hash<std::string>{}(ModuleContent);
```

This would make TUs with identical device IR produce the same HCF object ID → same cache keys → JIT once, reuse everywhere.

**Caveats:**
- Content hash must be computed AFTER all transformations (kernel outlining, argument expansion, etc.) but BEFORE writing to HCF
- May interact with the persistent cache (same ID = assumed identical content — must be true)
- Need to verify that `DeviceModule` is deterministic for the same source input (no randomness in optimization passes)
- Even without content-based HCF IDs, the runtime could add a secondary cache keyed by (kernel_name, target_arch, backend) that checks IR equality

## Alternative: Link-Time HCF Merging

A linker-level approach: after all TUs are compiled, merge HCF blobs that contain identical kernels. This could be implemented as:
1. Post-link pass that scans all HCF registration globals
2. Compares device IR content across HCF objects
3. Replaces duplicate HCF blobs with references to a single canonical copy

This is more complex but doesn't require changes to the compilation model.

## Files Referenced

- `src/compiler/sscp/TargetSeparationPass.cpp` — HCF object ID generation (line 509)
- `include/hipSYCL/runtime/kernel_configuration.hpp` — cache key composition (line 38)
- `include/hipSYCL/runtime/kernel_cache.hpp` — JIT cache lookup (line 316)
- `src/runtime/cuda/cuda_queue.cpp` — CUDA kernel dispatch config (line 556)
- `src/runtime/omp/omp_queue.cpp` — OMP kernel dispatch config (line 465)

## Test Program

`test_kernel_dedup.sh` + `test_kernel_dedup_{main,tu1,tu2}.cpp` in the SPUMA root. Compile with:
```bash
acpp -std=c++17 --acpp-targets=generic -c test_kernel_dedup_tu1.cpp -o tu1.o
acpp -std=c++17 --acpp-targets=generic -c test_kernel_dedup_tu2.cpp -o tu2.o
acpp -std=c++17 --acpp-targets=generic -c test_kernel_dedup_main.cpp -o main.o
acpp main.o tu1.o tu2.o -o test_kernel_dedup.exe
ACPP_DEBUG_LEVEL=3 ./test_kernel_dedup.exe 2>debug.log
grep "Registering kernel" debug.log
```

Expected: same kernel names from different HCF objects, each JIT'd separately.

## Confirmed Results (2026-03-29, 2026-03-30)

### Small test (2 TUs)

Rebuilt clang without stable naming patches. Compiled and ran the test:

**Without stable naming**: 16 registrations, **8 unique names** — kernel names are IDENTICAL across TUs.
The upstream `replaceInvalidMSABICharsInSymbolName` replaces `?`, `@`, `<`, `>` with `_`, which also strips TU-specific `?A0x<hash>` and `<lambda_N>` components. The remaining alphanumeric/underscore characters are deterministic across TUs.

**With stable naming** (previous build): 16 registrations, **8 unique names** (`__acpp_kernel_<hash>` format).

Both produce the same number of unique kernels. Stable naming is neither needed for correctness nor for deduplication.

### Full-scale test: SPUMA simpleFoam (107 DLLs, 168 EXEs)

Built with `WM_COMPILER=Sycl`, `ACPP_TARGETS=generic`, WITHOUT stable naming patches.
Ran simpleFoam on pitzDaily with `ACPP_DEBUG_LEVEL=3 ACPP_VISIBILITY_MASK=omp`.

| Metric | Value |
|--------|-------|
| HCF objects registered | **1,675** |
| Total kernel registrations | **27,484** |
| Unique kernel names | **1,958** |
| Average duplication factor | **14x** |
| simpleFoam runtime (OMP) | 315s |

**Top duplicated kernels:**

| Kernel (simplified) | Copies |
|---------------------|--------|
| `Field<scalar>::operator=(scalar)` | 285 |
| `Field<scalar>::operator=(UList)` | 219 |
| `Field<Vector>::operator=(Vector)` | 240 |
| `Field<Vector>::operator=(UList<Vector>)` | 194 |
| `Field<Tensor>::operator=(Tensor)` | 173 |

Each of these 285 copies has the SAME kernel name but comes from a different HCF object with a unique random ID. The runtime JIT cache key includes `hcf_object_id`, so each copy is JIT-compiled independently.

**JIT duplication is confirmed**: same kernels are JIT'd separately per HCF object due to random `hcf_object_id` in the cache key. This is an upstream architectural issue affecting all platforms.
