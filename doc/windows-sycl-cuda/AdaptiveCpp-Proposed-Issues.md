# Proposed Issues for AdaptiveCpp

Derived from building and running [SPUMA](https://gitlab.hpc.cineca.it/openfoam-on-gpu/SPUMA) (GPU-accelerated OpenFOAM) on Windows with AdaptiveCpp targeting CUDA via SYCL (MSVC ABI, Clang 21, SSCP backend).

**22 files patched**, git patch available: `AdaptiveCpp-MSVC-ABI.patch` (1992 lines). All patches are required for a working build — none are debugging artifacts.

---

## Issue 1: `sycl::reduction` silently returns 0 — pass-by-value bug in reduction_engine.hpp

**Type:** Bug (correctness)
**Severity:** High — silent data corruption, no error or warning
**Affects:** All backends (not Windows-specific)

### Description

The reduction engine in `include/hipSYCL/algorithms/reduction/reduction_engine.hpp` passes the reducer parameter **by value** in several internal lambdas/functors. The `combine()` call writes the accumulated result to a local copy, which is then discarded. The caller's reducer retains its initial value (typically 0).

This causes `sycl::reduction` to silently return 0 for any reduction operation (sum, min, max, etc.), with no error or diagnostic. The bug is extremely difficult to diagnose because the kernel appears to execute correctly — only the final result is wrong.

### Reproducer

Any SYCL program using `sycl::reduction` with the parallel_for overload:

```cpp
float result = 0;
{
    sycl::buffer<float> result_buf(&result, 1);
    q.submit([&](sycl::handler& cgh) {
        auto reducer = sycl::reduction(result_buf, cgh, sycl::plus<float>());
        cgh.parallel_for(sycl::range<1>(1024), reducer,
            [=](sycl::id<1> i, auto& red) {
                red.combine(1.0f);  // Should accumulate to 1024.0
            });
    });
}
// result == 0.0  (expected: 1024.0)
```

### Root Cause

In `reduction_engine.hpp`, the reducer is captured/passed by value in ~7 locations within the reduction implementation. Example (simplified):

```cpp
// BUG: reducer passed by value — combine() writes to a copy
auto final_reduction = [reducer](auto partial) {
    reducer.combine(partial);  // writes to local copy, discarded
};
```

### Fix

Change pass-by-value to pass-by-reference in all internal reduction forwarding. The fix is approximately 7 one-line changes (adding `&` to captures/parameters). We have a working patch.

### Impact

We discovered this while running a CFD solver (OpenFOAM) on CUDA via SYCL. The solver appeared to run but produced physically incorrect results. Diagnosing it required comparing field values between the SYCL backend and a known-good CPU path. This class of silent-corruption bug is particularly dangerous in scientific computing.

---

## Issue 2: MSVC ABI — ODR violations from lambda mangling in header-only templates

**Type:** Bug / Enhancement
**Affects:** Any build using MSVC ABI (Clang targeting MSVC, or MSVC itself)
**Related:** #1967 (clang-cl driver support)

### Description

MSVC mangles lambda types with translation-unit-specific identifiers:
- Anonymous namespace hashes: `?A0x` + 8 hex digits
- Lambda numbering: `<lambda_N>` where N varies per TU

When header-only template functions use lambdas (e.g., in `std::find_if`, `std::sort`, or SYCL kernel submissions), each translation unit that instantiates the template generates a **different mangled name** for the same logical lambda type. This causes:

1. **ODR violations** — the linker sees multiple incompatible definitions of the same template instantiation
2. **Linker errors** — duplicate symbol errors or unresolved externals depending on link order
3. **Runtime crashes** — if the linker picks one TU's instantiation but another TU calls it expecting a different lambda layout

### Affected Files (7)

The following headers contain lambdas in template functions that are instantiated across multiple TUs:

| File | Lambdas replaced | Key functions affected |
|------|------------------|----------------------|
| `include/hipSYCL/algorithms/reduction/reduction_engine.hpp` | ~15 | Entire reduction engine |
| `include/hipSYCL/runtime/data.hpp` | 11 | `data_region`, `allocation_list` |
| `include/hipSYCL/sycl/context.hpp` | 4 | `is_host()`, `get_platform()`, `get_devices()` |
| `include/hipSYCL/sycl/device.hpp` | 2 | `get_devices()`, `get_num_devices()` |
| `include/hipSYCL/runtime/device_list.hpp` | 2 | `unique_device_list::add()`, `get_num_backends()` |
| `include/hipSYCL/sycl/device_selector.hpp` | 4 | `aspect_selector()`, `select_devices()` |
| `include/hipSYCL/glue/reflection.hpp` | 0 | Comment cleanup only |

### Proposed Solution

Replace lambdas passed to template functions with **named functor structs** that have stable, predictable mangling across all TUs. Example:

```cpp
// BEFORE: Lambda with TU-specific mangling
template<typename T>
auto foo(std::vector<T>& v) {
    return std::find_if(v.begin(), v.end(), [](const T& x) { return x.valid(); });
}

// AFTER: Named functor with stable mangling
template<typename T>
struct validity_checker {
    bool operator()(const T& x) const { return x.valid(); }
};

template<typename T>
auto foo(std::vector<T>& v) {
    return std::find_if(v.begin(), v.end(), validity_checker<T>{});
}
```

This is a mechanical refactoring — no behavioral changes. We have a complete patch covering all 7 files (~1000 lines of changes, mostly boilerplate functor definitions).

### Why This Matters

Without this fix, AdaptiveCpp cannot be used from any build system that targets the MSVC ABI — which includes all native Windows C++ development (Visual Studio, clang-cl, Clang targeting MSVC). This blocks Windows adoption of AdaptiveCpp for any multi-TU project (i.e., any real-world project).

---

## Issue 3: SSCP kernel name mismatch on MSVC ABI — kernels not found at runtime

**Type:** Bug
**Affects:** SSCP compilation flow on MSVC ABI
**Related:** #1970 (static constexpr in SSCP on Windows)

### Description

In SSCP (Single-Source, Single-Pass) mode, the host compiler extracts kernel names from lambda function pointers, and the device separation pass collects kernel functions by name. On MSVC ABI, these names contain TU-specific identifiers that **differ between the host extraction pass and the device separation pass**:

- `?A0x` anonymous namespace hashes (8 hex digits, varies per TU/pass)
- `<lambda_N>` numbering (N varies depending on lambda declaration order in each pass)

**Result:** The host runtime looks up kernel `"?A0x1234ABCD@@<lambda_1>..."` but the device module contains `"?A0x5678CDEF@@<lambda_3>..."` → kernel not found → runtime failure.

### Proposed Solution

Canonicalize kernel names by stripping the unstable components and generating a deterministic hash:

1. Strip `?A0x........` anonymous namespace hashes
2. Normalize `<lambda_N>` → `<lambda>` (remove numbering)
3. Hash the canonicalized name + function signature with FNV-1a (64-bit)
4. Generate stable name: `__acpp_kernel_<16-hex-digits>`

This canonicalization must be applied identically in both:
- **Host side:** `HostKernelNameExtractionPass.cpp` — when extracting kernel names for the runtime lookup table
- **Device side:** `TargetSeparationPass.cpp` — when collecting and naming kernel functions in the device module

### Files Changed (3)

| File | Change |
|------|--------|
| `include/hipSYCL/compiler/llvm-to-backend/NameHandling.hpp` | +94 lines: `canonicalizeMSVCKernelName()`, `generateStableMSVCKernelName()`, `fnv1aHash()` |
| `src/compiler/sscp/HostKernelNameExtractionPass.cpp` | +22 lines: Apply stable naming during host extraction |
| `src/compiler/sscp/TargetSeparationPass.cpp` | +53 lines: `applyStableMSVCKernelNaming()` + broadened `_MSC_VER` check to include Clang targeting MSVC |

All changes are guarded by `#ifdef ACPP_MSVC_ABI_TARGET` (defined as `_MSC_VER || (_WIN32 && !__MINGW32__)`) — zero impact on Linux/macOS builds.

### Additional Note

The existing `#ifdef _MSC_VER` guard in `TargetSeparationPass.cpp` is too narrow — it doesn't match Clang targeting MSVC ABI (which defines `_WIN32` but not `_MSC_VER` unless using clang-cl). We broadened it to `#if defined(_MSC_VER) || (defined(_WIN32) && !defined(__MINGW32__))`.

---

## Issue 4: Windows build system — CMake, static init order, and platform portability

**Type:** Enhancement
**Affects:** Building AdaptiveCpp on Windows with Clang (MSVC ABI or MinGW)

### Description

Building AdaptiveCpp on Windows (tested with Clang 21 from MSYS2, targeting MSVC ABI) requires several build system and platform fixes. These are all standard portability issues — no architectural changes needed.

### 4a. CMake: PE export limit and library linking (5 files + 1 new)

**Problem:** The compiler plugin DLL (`acpp-clang.dll`) tries to export all symbols via `WINDOWS_EXPORT_ALL_SYMBOLS`. LLVM alone has 77,000+ symbols; the PE format limit is 65,535. Also, static LLVM library linking requires explicit component lists.

**Fix:**
- `src/compiler/CMakeLists.txt`: Explicit Clang/LLVM component library list, `WINDOWS_EXPORT_ALL_SYMBOLS Off` + `--exclude-all-symbols` + minimal `.def` file
- `src/compiler/llvm-to-backend/CMakeLists.txt`: Skip `libLLVM.so` check on Windows, add `pthread` for MinGW
- `src/runtime/CMakeLists.txt`: Disable LTO on MinGW, fix OpenMP linking
- `src/common/CMakeLists.txt`: Broaden `WIN32` checks to include `MINGW OR MSYS`
- `src/CMakeLists.txt`: Guard `-Bsymbolic-functions` with `CMAKE_SYSTEM_NAME STREQUAL "Linux"` (ELF-only)
- New `src/compiler/acpp-clang.def`: Minimal export list (6 symbols) for plugin registration

### 4b. Static initialization order (2 files + 1 new)

**Problem:** On Windows DLLs, the linker doesn't preserve object file order for static constructors. Header-injected static objects (`persistent_runtime_object`, `__acpp_register_sscp_hcf_object`) can initialize before the runtime singletons they depend on → crash at startup.

**Fix:**
- New `include/hipSYCL/common/init_priority.hpp`: Defines `ACPP_INIT_PRIORITY_RUNTIME_CORE` (101) and `ACPP_INIT_PRIORITY_USER` (65534) using `__attribute__((init_priority(N)))`
- `include/hipSYCL/glue/persistent_runtime.hpp`: Apply `ACPP_INIT_PRIORITY_USER`
- `include/hipSYCL/glue/llvm-sscp/hcf_registration.hpp`: Apply `ACPP_INIT_PRIORITY_USER`

### 4c. Platform portability (5 files)

| File | Issue | Fix |
|------|-------|-----|
| `src/runtime/omp/omp_queue.cpp` | `WIN32` not defined by Clang (uses `_WIN32`) | Check both `WIN32` and `_WIN32` |
| `include/hipSYCL/compiler/cbs/MathUtils.hpp` | POSIX `ffs()` not available on MSVC | Use `__builtin_ffs()` (GCC/Clang builtin) |
| `include/hipSYCL/sycl/libkernel/host/builtins.hpp` | MSVC narrowing conversion warning | `T{1}` → `T(1)` |
| `src/libkernel/sscp/host/math.cpp` | MSVC needs `_USE_MATH_DEFINES` for `M_PI` | Add `#define _USE_MATH_DEFINES` before `<cmath>` |
| `src/compiler/llvm-to-backend/ptx/LLVMToPtx.cpp` | MSVC-mangled names contain chars invalid in PTX | Sanitize symbol names in PTX module |

### Availability

We have a complete git patch (1992 lines, 22 files) covering all changes. Happy to submit as a PR or series of PRs if there's interest. The patch has been validated by building and running a large-scale CFD application (106 DLLs, 144 executables) on Windows with both CPU (OpenMP) and GPU (CUDA via SSCP JIT) backends.

### Environment

- **OS:** Windows 11 Pro
- **Compiler:** Clang 21 (built from source, targeting MSVC ABI)
- **LLVM:** 21 (static libraries)
- **CUDA:** 12.5
- **GPU:** NVIDIA RTX 3060 Laptop (sm_86)
- **Shell:** MSYS2
- **AdaptiveCpp target:** `generic` (SSCP JIT → `llvm-to-ptx` at runtime)

---

## Submission Strategy

Four separate PRs, ordered by review complexity:

1. **PR 1: Reduction correctness fix** (Issue 1) — ~7 line changes, standalone bug fix, fast to review
2. **PR 2: Platform portability + build system** (Issue 4) — 13 files, CMake + small fixes, no API changes
3. **PR 3: SSCP kernel naming for MSVC ABI** (Issue 3) — 3 files, ~170 lines, compiler passes only, guarded by `#ifdef`
4. **PR 4: Lambda-to-functor refactoring** (Issue 2) — 7 files, ~1000 lines, largest change but mechanical refactoring
