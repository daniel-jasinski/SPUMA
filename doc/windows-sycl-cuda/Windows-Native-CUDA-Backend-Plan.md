# Windows Native CUDA Backend

## Naming Convention

Windows GPU backends use **GPU API names** with MSVC ABI implicit (the only viable ABI for GPU computing on Windows):

| Backend | `WM_COMPILER` | Flag | Rules dir | Platform dir |
|---------|--------------|------|-----------|-------------|
| SYCL | `Sycl` | `have_sycl` | `win64Sycl` | `win64SyclDPInt32Opt` |
| CUDA | `Cuda` | `have_cuda` | `win64Cuda` | `win64CudaDPInt32Opt` |
| HIP | `Hip` | `have_hip` | `win64Hip` | `win64HipDPInt32Opt` |

This diverges from Linux naming (`Nvidia`, `Amdclang`) because on Windows all backends use `clang++` with different flags — the GPU API differentiates them, not the compiler.

## Compilation Model

**ALL `.C` files compiled with `-x cuda -fgpu-rdc`** — kernel code propagates through headers via the NoRepository pattern:

```
Field.H → executors.H → cudaExecutor.cuh → #include "cudaExecutor.cu" (NoRepository)
```

Any `.C` file that includes `Field.H` or `lduMatrix.H` contains CUDA `__global__` kernel templates. A split model (host-only `.C` + CUDA `.cu`) does NOT work because the template definitions must be visible at every call site for instantiation.

Key flags:
- **`-x cuda`**: treat all source files as CUDA (kernel code visible everywhere)
- **`-fgpu-rdc`**: relocatable device code — defers fatbin registration to link time. Without this, every `.o` file registers a fatbin at DLL load → `STATUS_DLL_INIT_FAILED (0xC0000142)`
- **`-w`**: suppress all warnings — Clang's CUDA device pass generates `__declspec` warnings that silently cause `.o` output to be dropped (Clang bug: exit 0 but no file produced)

## Linker: lld-link (LLVM)

Must use `lld-link` instead of MSVC `link.exe`:
- MSVC `link.exe` fails with **LNK1227** on `-fgpu-rdc` weak `.offloading.entry` symbols
- `lld-link` handles these with `/force:multiple`
- `/force:unresolved` needed for ~30 dummy Pstream symbols (same as SYCL build)
- Must add `/LIBPATH:$CUDA_PATH/lib/x64` explicitly — `lld-link` doesn't inherit `LIB` env var like MSVC `link.exe`
- Import lib naming: `lld-link` creates `libFoo.lib` from `libFoo.dll`, wmake's copy step creates `Foo.lib` (without `lib` prefix) for downstream `-lFoo` references

## Source Code Fixes Required

### 1. `extern "C" __device__` on NVIDIA device math declarations

Clang requires explicit `__device__` annotation on `__nv_*` function declarations. Without it: `error: reference to __host__ function '__nv_fabs' in __host__ __device__ function`.

Files changed:
- `doubleScalar.H` — all `__nv_*` declarations (24 functions + 6 Bessel)
- `floatScalar.H` — all `__nv_*f` declarations (24 functions + 6 Bessel)
- `doubleFloat.H` — `__nv_pow`, `__nv_powf`
- `complexI.H` — 18 `__nv_*` declarations + `FOAM_DEVICE` on ~20 complex math functions

### 2. `cudaAtomic.cuh` — `FOAM_DEVICE` + `#ifdef __CUDA_ARCH__`

CUDA atomic intrinsics (`atomicAdd`, `atomicMin`, `atomicCAS`) are device-only builtins. Functions calling them must use `FOAM_DEVICE` annotation with `#ifdef __CUDA_ARCH__` guards around the actual intrinsic calls, so the host compilation pass sees valid (empty) function bodies.

### 3. `cudaDeviceUtils.cuh` — `__CUDACC__` guard

Changed from `#ifdef have_cuda` to `#if defined(have_cuda) && defined(__CUDACC__)`. Prevents device intrinsics (`__double_as_longlong`, `atomicCAS`) from being visible during compilation passes where `__CUDACC__` is not defined.

## Build System Changes

### `wmake/rules/win64Cuda/` (5 files)

- `general` — Windows extensions (`.dll`, `.exe`), no `-ldl`
- `c++` — Clang CUDA flags, lld-link, cudart linking
- `c++Opt`, `c++Debug` — optimization flags
- `c` — C compiler (same as SYCL)

### `etc/bashrc`

Auto-detection: `Cuda*) have_cuda=true`, `Hip*) have_hip=true`

### `src/Allwmake`

- 3-pass Pstream build for `WM_OSTYPE=MSwindows` (was only `Mingw*`)
- `libPstream_static.lib` copy step (wmake `libo` creates `libPstream.lib`, Make/options expects `libPstream_static.lib`)

### `wmake/wmake`

Added `NVARCH` and `CUDA_PATH_SHORT` to all make variable passing (3 locations + updatedep).

## Current Build Status (2026-03-24)

- **22+ DLLs, 17+ EXEs** built (build in progress)
- **libOpenFOAM.dll** — 12.9 MB, links successfully with `lld-link`
- **3702 `.o` files** compiled across all libraries
- **`WM_NCOMPPROCS=4`** — CUDA dual-pass compilation uses ~2x memory per process
- Build script: `rebuild-cuda-inline.sh`, redirect must be inside MSYS2 command (Git Bash buffering loses output)

## Known Issues

1. **`/force:unresolved` required** — ~30 dummy Pstream symbols are intentionally unresolved (same as SYCL build). Downstream DLLs that use executor templates also have CUDA runtime symbols that resolve via cudart.lib.
2. **`/force:multiple` required** — `-fgpu-rdc` creates duplicate `.offloading.entry` weak symbols when the same kernel template is instantiated in multiple `.o` files.
3. **`foamConfig.Cver.cc`** — intermittent "untrusted mount point" error when lnInclude symlinks cross into the build directory. Workaround: compile manually from `src/OpenFOAM/` working directory.
4. **CGAL-dependent utilities** — `ROMmodelNew.C`, `PDRobstacleIO.C` etc. have old RTS table pointer pattern (`constructorTablePtr_->` instead of `constructorTablePtr_()->`). Not built in SYCL build either.

## Runtime Testing

Not yet tested — build must complete first. Test plan:
1. `blockMesh` on pitzDaily
2. `simpleFoam` on pitzDaily (CPU mode first, then GPU with `-pool fixedSizeMemoryPool -poolSize 1`)
3. Compare numerical results with SYCL backend

Environment for testing:
```cmd
set PATH=C:\PROGRA~1\NVIDIA~2\CUDA\v12.5\bin;C:\OpenFOAM\SPUMA\platforms\win64CudaDPInt32Opt\lib;C:\OpenFOAM\SPUMA\platforms\win64CudaDPInt32Opt\lib\dummy;C:\OpenFOAM\SPUMA\platforms\win64CudaDPInt32Opt\bin;%PATH%
set WM_PROJECT_DIR=C:\OpenFOAM\SPUMA
set WM_PROJECT=OpenFOAM
set FOAM_ETC=C:\OpenFOAM\SPUMA\etc
```
