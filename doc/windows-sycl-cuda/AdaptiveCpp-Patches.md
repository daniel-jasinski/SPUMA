# AdaptiveCpp Patches for Windows MSVC ABI + CUDA

All changes applied to AdaptiveCpp (source: `C:\dev\AdaptiveCpp-dev`, install: `C:\dev\llvm-acpp-msvc-21`) to build and run SPUMA on Windows with MSVC ABI targeting CUDA via SYCL.

**11 files changed, 360 lines. No LLVM/Clang patches required.**

> **Note:** Category 2 (Stable Kernel Naming, 3 files) was removed — it caused a JIT cache
> collision by stripping `<lambda_N>` ordinals, making different lambdas in the same function
> hash-identical. Upstream's `replaceInvalidMSABICharsInSymbolName()` is sufficient.

---

## Category 1: MSVC ABI Lambda-to-Functor Refactoring (7 files)

### Problem

MSVC mangles lambda types with TU-specific identifiers (anonymous namespace hashes `?A0x...`, lambda numbering `<lambda_N>`). When the same template is instantiated in multiple translation units, each TU generates a different mangled name for the same lambda → ODR violations → linker errors or runtime crashes.

### Solution

Replace lambdas passed to template functions with named functors that have stable, predictable mangling across all TUs.

### Files Changed

**`include/hipSYCL/algorithms/reduction/reduction_engine.hpp`** (+596/-271)
- All lambdas in the reduction engine → named functors
- **Also Fix #43**: pass-by-value → pass-by-reference for reducer parameters (7 lines). Without this, `combine()` writes to a copy and reductions silently return 0. This is a correctness bug introduced during the refactoring.
- Still necessary: **YES** (both the functor refactoring and the reference fix)

**`include/hipSYCL/runtime/data.hpp`** (+257/-93)
- 11 named functor classes replacing lambdas in `data_region` and `allocation_list`: `data_region_cleanup_functor`, `noop_allocation_handler`, `remove_invalid_pages_handler`, `update_invalid_pages_handler`, `get_intersections_handler`, `find_update_sources_handler`, `get_memory_handler`, `copy_allocation_handler`, `memory_selector`, `check_valid_pages_handler`
- Still necessary: **YES**

**`include/hipSYCL/sycl/context.hpp`** (+77/-30)
- 4 named functors: `is_host_checker`, `device_collector`, `hash_code_accumulator`, `platform_finder`
- Replaces lambdas in `is_host()`, `get_platform()`, `get_devices()`, `AdaptiveCpp_hash_code()`
- Still necessary: **YES**

**`include/hipSYCL/sycl/device.hpp`** (+61/-31)
- `backend_device_enumerator` functor replacing lambda in `device::get_devices()`
- Moved `get_devices()` and `get_num_devices()` out-of-line (after functor definition) to avoid incomplete type issues
- Still necessary: **YES**

**`include/hipSYCL/runtime/device_list.hpp`** (+28/-6)
- `device_adder` and `backend_counter` functors replacing lambdas in `unique_device_list::add()` and `get_num_backends()`
- Still necessary: **YES**

**`include/hipSYCL/sycl/device_selector.hpp`** (+97/-41)
- `aspect_list_selector`, `variadic_aspect_selector` classes replacing `aspect_selector()` lambdas
- `device_score_calculator`, `device_score_comparator` functors for `select_devices()` (replaces `std::transform`/`std::sort` lambdas)
- Still necessary: **YES**

**`include/hipSYCL/glue/reflection.hpp`** (-6 lines)
- Removed comments about compiler opaque pointer workaround and `extern "C"` declaration comment
- Still necessary: **cosmetic only** (comments removed, no functional change)

---

## ~~Category 2: MSVC ABI Stable Kernel Naming (REMOVED)~~

**REMOVED.** This patch stripped `<lambda_N>` ordinals and hashed kernel names to `__acpp_kernel_<hex>`. It caused a fatal JIT cache collision: different lambdas in the same function (e.g., `<lambda_0>` add vs `<lambda_1>` multiply) hashed to the same key, so the runtime always executed the first lambda's code for all dispatches.

Upstream's `#ifdef _MSC_VER` code in `HostKernelNameExtractionPass.cpp` and `TargetSeparationPass.cpp` uses `replaceInvalidMSABICharsInSymbolName()` which replaces `?@<>` with `_` while preserving lambda ordinals. This is sufficient for kernel name matching.

---

## Category 3: Windows Build System (CMake) (5 files)

### Problem

AdaptiveCpp's CMake build assumed either Linux or native MSVC (`_MSC_VER`). Building with Clang targeting MSVC ABI from MSYS2/MinGW required fixes for library detection, linking, and PE export limits.

### Files Changed

**`src/compiler/CMakeLists.txt`** (+60/-3)
- Added explicit Clang/LLVM component library list for Windows linking (clangFrontend, clangDriver, LLVMSupport, etc.) — replaces bare `clang LLVMSupport` which doesn't work with static LLVM
- Added `LINK_COMPONENTS Passes IPO` for LLVM pass infrastructure
- PE export limit fix: `WINDOWS_EXPORT_ALL_SYMBOLS Off` + `--exclude-all-symbols` + minimal `.def` file for plugin registration (LLVM has 77000+ symbols, PE limit is 65535)
- `SYSTEM` include flag removed for MINGW/MSYS (conflicts with MinGW system headers)
- Still necessary: **YES** (PE export limit is fundamental)

**`src/compiler/llvm-to-backend/CMakeLists.txt`** (+41/-3)
- Added `pthread` library for MinGW static linking
- Skip `libLLVM.so` check on Windows (uses static libraries)
- `SYSTEM` include flag removed for MINGW/MSYS
- Still necessary: **YES** (though has duplicate `pthread` additions that should be cleaned up)

**`src/runtime/CMakeLists.txt`** (+22/-5)
- Disabled LTO on MinGW (causes DLL export issues)
- `--export-all-symbols` for MinGW (WINDOWS_EXPORT_ALL_SYMBOLS unreliable in LLVM component mode)
- OpenMP linking fix: `OpenMP::OpenMP_CXX` target doesn't work on MSYS/MinGW, use flags directly
- Still necessary: **YES**

**`src/common/CMakeLists.txt`** (+2/-2)
- Broadened `WIN32` checks to include `MINGW OR MSYS`
- Fixed `MinGW` → `MinGW OR MSYS` for output name logic
- Still necessary: **YES**

**`src/CMakeLists.txt`** (+1/-1)
- Changed `NOT APPLE AND NOT WIN32` to `CMAKE_SYSTEM_NAME STREQUAL "Linux"` for `-Bsymbolic-functions` (only valid on Linux ELF)
- Still necessary: **YES**

**New file: `src/compiler/acpp-clang.def`**
- Minimal symbol export list for the compiler plugin DLL (6 symbols)
- Plugin registration entries (`PluginASTAction` registry, `llvmGetPassPluginInfo`)
- Overcomes 65535 PE export limit
- Still necessary: **YES**

---

## Category 4: Windows Static Init Order (2 files + 1 new)

### Problem

On Windows, the LLVM linker doesn't preserve object file order for static constructors like GNU ld does. Header-injected static objects (runtime keep-alive tokens, HCF registration) can initialize before the runtime singletons they depend on → SIOF crash.

### Files Changed

**New file: `include/hipSYCL/common/init_priority.hpp`**
- Defines `ACPP_INIT_PRIORITY_RUNTIME_CORE` (priority 101) and `ACPP_INIT_PRIORITY_USER` (priority 65534)
- Uses GCC/Clang `__attribute__((init_priority(N)))`, no-op on other platforms
- Still necessary: **YES**

**`include/hipSYCL/glue/persistent_runtime.hpp`** (+2/-1)
- `persistent_runtime_object` → `ACPP_INIT_PRIORITY_USER persistent_runtime_object`
- Still necessary: **YES**

**`include/hipSYCL/glue/llvm-sscp/hcf_registration.hpp`** (+2/-1)
- `__acpp_register_sscp_hcf_object` → `ACPP_INIT_PRIORITY_USER __acpp_register_sscp_hcf_object`
- Still necessary: **YES**

---

## Category 5: Windows Platform Fixes (5 files)

Small fixes for MSVC/Windows platform compatibility.

**`src/runtime/omp/omp_queue.cpp`** (+4/-4)
- `#ifndef WIN32` → `#if !defined(WIN32) && !defined(_WIN32)` (Clang targeting MSVC defines `_WIN32` but not `WIN32`)
- `#include <Windows.h>` → `#include <windows.h>` (case-sensitive filesystems)
- Still necessary: **YES**

**`include/hipSYCL/compiler/cbs/MathUtils.hpp`** (+3/-2)
- `ffs()` (POSIX) → `__builtin_ffs()` (GCC/Clang builtin) with zero guard
- Still necessary: **YES** (MSVC doesn't have POSIX `ffs()`)

**`include/hipSYCL/sycl/libkernel/host/builtins.hpp`** (+1/-1)
- `T{1}/T{y}` → `T(1)/static_cast<T>(y)` in `__acpp_rootn` — avoids narrowing conversion warning on MSVC
- Still necessary: **YES** (cosmetic but prevents -Werror build failures)

**`src/libkernel/sscp/host/math.cpp`** (+1)
- Added `#define _USE_MATH_DEFINES` before `#include <cmath>` — MSVC requires this for `M_PI`, `M_E`, etc.
- Still necessary: **YES**

**`src/compiler/AdaptiveCppLlvmPasses.cpp`** + **`include/hipSYCL/compiler/GlobalsPruningPass.hpp`** + **`src/compiler/GlobalsPruningPass.cpp`** (+3/-3)
- `#if !defined(_WIN32) || defined(ACPP_LLVM_COMPONENT)` → `#if 1` — enables new pass manager pass on Windows
- The original guard disabled the new pass manager code on Windows unless built as an LLVM component. With our MinGW build, both paths are needed.
- Still necessary: **YES** (but `#if 1` is ugly — could use a proper condition)

~~**`src/compiler/llvm-to-backend/ptx/LLVMToPtx.cpp`** (REMOVED)~~
- Previously updated the hardcoded NVPTX `DataLayout` string for LLVM 21. Verified that LLVM 21 accepts the old upstream layout string and produces identical PTX — the patch was unnecessary.

---

## Summary

| Category | Files | Still needed | Could be upstreamed |
|----------|-------|-------------|-------------------|
| Lambda→functor (MSVC ABI) | 7 | All | Yes — benefits any MSVC ABI build |
| Stable kernel naming | 3 | All | Yes — required for SSCP on MSVC ABI |
| CMake build system | 5+1 | All | Yes — enables Windows builds |
| Static init order | 2+1 | All | Yes — fixes SIOF on Windows |
| Platform fixes | 5 | All | Yes — standard portability fixes |

**None of these are debugging artifacts.** All 22 file changes are required for a working AdaptiveCpp on Windows with MSVC ABI + CUDA.

The reduction_engine.hpp by-reference fix (Fix #43) is a **correctness bug** in the lambda→functor refactoring — it affects all backends, not just Windows.
