# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## CRITICAL SAFETY RULES

**DO NOT DELETE FILES OR DIRECTORIES without explicit user confirmation.**

- NEVER use `rm -rf` on project directories or parent directories
- NEVER use `rmdir /s /q` on project directories
- If cleanup is needed, ASK THE USER FIRST and let them do it manually
- MSYS2 path translation (`/c/` to `C:\`) can cause unexpected deletions
- When in doubt, use `ls` or `dir` to verify paths before any destructive operation

**Recovery information:**
- Repository: `git@github.com:daniel-jasinski/SPUMA.git`
- Branch: `sycl-backend`
- All Windows DLL fixes are documented in `doc/windows-sycl-cuda/`

---

## Project Overview

SPUMA (Simulation Processing in Unified Memory Accelerators) is a GPU-accelerated CFD software based on OpenFOAM-v2412, developed by Cineca. It implements GPU porting of OpenFOAM targeting NVIDIA (CUDA), AMD (HIP), and Intel GPUs using a unified memory approach.

## Windows MSVC+CUDA Build

**See `doc/windows-sycl-cuda/` for complete documentation:**
- `SPUMA-Windows-CUDA-Build-Guide.md` - Complete build guide
- `Windows-DLL-Export-Fixes.md` - All required code changes
- `Runtime-Selection-Table-Fix.md` - Function-based table access pattern
- `Current-Status-Next-Steps.md` - Implementation checklist
- `Debugging-Conclusions.md` - Root causes, fixes, and diagnostic techniques (Fixes #1-#6)
- `Agent-Handoff-Spec.md` - Handoff specification for continuing DLL init debugging

### Quick Start Configuration

**SYCL backend (AdaptiveCpp SSCP — generic LLVM IR, JIT to CUDA/CPU):**
```bash
WM_COMPILER=Sycl WM_LABEL_SIZE=32
export have_sycl=true
export ACPP_PATH="/c/dev/llvm-acpp-msvc-21"
export ACPP_TARGETS=generic
```

**Native CUDA backend (Clang -x cuda — AOT PTX for NVIDIA):**
```bash
WM_COMPILER=Cuda WM_LABEL_SIZE=32 NVARCH=86
export CUDA_PATH_SHORT="$(cygpath -m -s 'C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5')"
export FOAM_LINK_DUMMY_PSTREAM=libo
```

**Native HIP backend (AMD ROCm Clang -x hip — AOT for AMD GPUs):**
```bash
WM_COMPILER=Hip WM_LABEL_SIZE=32 AMDARCH=gfx1030
export HIP_PATH_SHORT="$(cygpath -m -s 'C:/Program Files/AMD/ROCm/7.1')"
export FOAM_LINK_DUMMY_PSTREAM=libo
```

**CUDA 12.5 required** - CUDA 13.0+ is NOT supported (Clang compatibility).

## Build Commands

### Environment Setup
```bash
source etc/bashrc
```

### Build
```bash
./Allwmake -j -s -q -l   # Full parallel build
cd src/OpenFOAM && wmake # Single library
```

### Clean
```bash
./Allwclean              # Full clean
wclean                   # Current directory
```

## GPU Runtime
```bash
<solver>Foam -pool fixedSizeMemoryPool -poolSize 32   # GPU with 32GB pool
```

## Architecture

### GPU Abstraction (`src/OpenFOAM/device/`)
- `executor/` - Parallel execution (CRTP pattern)
- `memoryExecutor/` - Unified memory management
- `syclGlobal.C` - SYCL queue/device init

### Memory Pools (`src/OpenFOAM/memoryPool/`)
- `dummyMemoryPool` - Individual allocations (default)
- `fixedSizeMemoryPool` - Pre-allocated (GPU-optimized)
- `umpireMemoryPool` - LLNL Umpire library

## Commit Guidelines

- No AI-generated annotations in commit messages
- Do not commit CLAUDE.md or .claude/ directories

## Debugging Conclusions

**IMPORTANT**: When debugging build failures, runtime crashes, hangs, or any non-trivial issue, you MUST record your findings in `doc/windows-sycl-cuda/Debugging-Conclusions.md`. Include:
- The symptom (what you observed)
- The root cause (what was actually wrong)
- The fix (what was changed)
- The lesson (reusable insight for future debugging)

This is critical for continuity between agent sessions. Read the existing entries before debugging to avoid re-investigating solved problems. Append new entries following the template at the bottom of that document.

You should also regularly update the `Current Build Status` section with the latest state of the build, including what DLLs have been built, what's currently working, and what the next steps are.

## Important Notes

- Use `fixedSizeMemoryPool` for GPU runs
- Current branch: `sycl-backend`
- Target: CUDA via SYCL/AdaptiveCpp

---

## Current Build Status (2026-03-13)

### What's Built

**106 DLLs** and **144 EXEs** in `platforms/win64MsvcSyclDPInt32Opt/`

#### Core libraries (all built)
- ✅ `libOSspecific.lib`, `libPstream_static.lib`, `libPstream.dll` (dummy)
- ✅ **`libOpenFOAM.dll`**, `libfiniteVolume.dll`, `libfiniteArea.dll`
- ✅ `libfileFormats.dll`, `libsurfMesh.dll`, `libmeshTools.dll`, `libblockMesh.dll`
- ✅ `libextrudeModel.dll`, `libdynamicMesh.dll`, `libdynamicFvMesh.dll`
- ✅ `libsnappyHexMesh.dll`, `libconversion.dll`, `libfvMotionSolvers.dll`

#### Transport, turbulence, thermo (all built)
- ✅ All transport models (incompressible, compressible, twoPhaseMixture, etc.)
- ✅ All turbulence models (incompressible, compressible, schemes)
- ✅ All thermophysical models (specie, basic, reactionThermo, radiation, etc.)
- ✅ `libcombustionModels.dll`, `libODE.dll`

#### Phase system models (all built)
- ✅ reactingEuler (saturation, multiphase, twoPhase, turbulence)
- ✅ multiphaseInter, multiphaseEuler, twoPhaseEuler, twoPhaseInter

#### Lagrangian, region models, function objects (all built)
- ✅ All lagrangian libs (basic, intermediate, turbulence, spray, DSMC, coalCombustion, molecularDynamics)
- ✅ All region models (regionModel, pyrolysis, surfaceFilm, thermalBaffle, regionCoupling)
- ✅ All function objects (field, forces, initialisation, utilities, solvers, phaseSystems, lagrangian)
- ✅ `libfaOptions.dll`, `libregionFaModels.dll`, `libthermoTools.dll`

#### Mesh, motion, other (all built)
- ✅ `liboverset.dll`, `libfvOptions.dll`, `libsampling.dll`, `libatmosphericModels.dll`
- ✅ `libtopoChangerFvMesh.dll`, `libwaveModels.dll`, `libengine.dll`
- ✅ `libsixDoFRigidBodyMotion.dll`, `librigidBodyDynamics.dll`, `librigidBodyMeshMotion.dll`
- ✅ `libinterfaceTrackingFvMesh.dll`, `libgenericPatchFields.dll`
- ✅ Decomposition: `libdecompositionMethods.dll`, `libdecompose.dll`, `librenumberMethods.dll`, all dummyThirdParty stubs
- ✅ Parallel: `libdistributed.dll`, `libreconstruct.dll`, `libfaDecompose.dll`, `libfaReconstruct.dll`
- ✅ Utility sub-libs: `libhelpTypes.dll`, `libalphaFieldFunctions.dll`, `libtabulatedWallFunctions.dll`, `libsurfaceFeatureExtract.dll`

#### Adjoint (built with generic backend)
- ✅ `libadjointOptimisation.dll` — builds with `ACPP_TARGETS=generic` (CUDA AOT had LLVM circular dependency bug)

#### Solvers (8 built)
- ✅ simpleFoam, icoFoam, pisoFoam, SRFSimpleFoam, laplacianFoam, potentialFoam
- ✅ pimpleFoam, rhoPimpleFoam

#### Utilities (136 built)
- ✅ blockMesh, PDRblockMesh, extrude2DMesh, checkMesh, checkFaMesh, makeFaMesh
- ✅ Most mesh manipulation (topoSet, transformPoints, splitMeshRegions, createBaffles, renumberMesh, etc.)
- ✅ Most mesh conversion (gmshToFoam, ensightToFoam, star4ToFoam, ansysToFoam, fluent3DMeshToFoam, fluentMeshToFoam, gambitToFoam, etc.)
- ✅ snappyHexMesh, decomposePar, redistributePar, mapFields, surfaceRedistributePar
- ✅ foamHelp, setAlphaField, wallFunctionTable, surfaceFeatureExtract, surfacePatch
- ✅ Most surface utilities, pre/post-processing, thermophysical (chemkinToFoam)

#### Not built (expected)
- ❌ CGAL-dependent utilities (viewFactorsGen, surfaceBooleanFeatures) — require CGAL (not installed)
- ❌ FFTW3-dependent utilities (noise, boxTurb) — require FFTW3 + randomProcesses lib (not installed)

### Fixes Applied (51 total)

All fixes documented in `doc/windows-sycl-cuda/Debugging-Conclusions.md`. Key recent:
- Fix #41: `MemoryPool::New()` replaces auto-created dummyMemoryPool with requested fixedSizeMemoryPool. Previously CUDA kernels accessed non-USM memory → `CUDA:700`.
- Fix #42: Exit-time crash with CUDA backend — `_exit(0)` workaround. Solver runs correctly but process exit triggers CUDA cleanup segfault.
- Fix #43: CUDA backend sycl::reduction returning 0. Patched AdaptiveCpp `reduction_engine.hpp` (pass-by-reference). USM-based reduction in syclExecutor.cpp.
- Fix #44: Spurious debug output from NoRepository templates. ~56 files changed: `debug` → `debug_()` in all cross-DLL template code. Removed diagnostic fprintf traces.
- Fix #45: RTS registration used `typeName_()` (generic template name) instead of `typeName` (per-specialization name). LimitedScheme, gradScheme etc. registered under wrong keys. Hybrid fix: keep `typeName_()` as global default (safe for cross-DLL), pass explicit names in specific macros (LimitedScheme.H, PhiScheme.H, multivariateScheme.H). Duplicates: 458 → 13.
- Fix #46: Compound<T> duplicate RTS entries. Deleted 11 stale `.o` files compiled before `(#Type)` was added to `addCompoundToRunTimeSelectionTable`. Compound<T> duplicates: 12 → 0. Remaining 459 Reaction duplicates are by-design OpenFOAM behavior (harmless).
- Fix #47: Failed DLL load destroys RTS tables. `LoadLibrary` failure triggers DLL unload → static destructors delete shared RTS tables → SIGSEGV. Fix: `Foam::RTS_TABLE_CLEANUP = false` on Windows (no-op in `construct(false)`). Safe since solvers use `_exit(0)`.
- Fix #48: Remaining 4 RTS duplicates. liquidThermo.C → `addToRunTimeSelectionTableKey` with explicit names. Stale `libcompressibleTurbulenceModels.dll` (Feb 8) rebuilt with current headers. **0 duplicate warnings in simpleFoam.**
- Fix #49: IO class names for stock OpenFOAM interop. CompactIOList/CompactIOField wrote generic class names (`List`, `Field`) instead of per-specialization names (`faceList`, `faceCompactList`). Fix: set `headerClassName()` before `regIOobject::writeObject` (bypasses SPUMA's `type()` which returns `typeName_()`). Reverted `typeName_()` → `typeName` in read paths. Meshes now interoperable with stock OpenFOAM v2506.
- Fix #50: IOField/IOList/GlobalIOField writeObject override for correct per-specialization class names. After rebuilding libOpenFOAM.dll, MUST rebuild downstream DLLs (COMDAT folding changes). All debug traces removed from simpleFoam.C and UEqn.H.

### Current State

- **Generic SSCP backend (ACPP_TARGETS=generic) fully working.** Single binary runs on CPU (OMP) and NVIDIA GPU (CUDA JIT).
- **blockMesh works!** Exit 0. Correct mesh class names (`vectorField`, `faceList`, `labelList`).
- **simpleFoam works!** Exit 0. Converges in 281 iterations (pitzDaily, fresh run). Clean output, **0 RTS duplicate warnings**.
- **simpleFoam OMP**: 3 iterations (restart), correct residuals.
- **simpleFoam CUDA JIT**: 3 iterations (restart), correct residuals, fixedSizeMemoryPool 1GB. First iteration ~6.5s (JIT), subsequent ~4s. Residuals match OMP exactly.
- **Restart works!** Exit 0.
- **106 DLLs, 144 EXEs built.** Full SPUMA library suite + utilities + adjoint compiled.
- **CUDA backend tested**: RTX 3060 Laptop (sm_86), 1GB pool, pitzDaily case — solver converges, all GPU kernels pass.
- **Diagnostic traces removed**: All debug fprintf removed from source files, simpleFoam.C, and UEqn.H.
- **RTS duplicate warnings eliminated**: Fix #48 resolved all remaining duplicate warnings (was 1237, now 0).
- **Binary size**: 1,213 MB total DLLs (generic). Essentially identical to cuda:sm_86 AOT (1,212 MB). Earlier ~950 MB prediction was incorrect.
- **Known issue**: `phi` (surfaceScalarField) writes `class GeometricField;` instead of `class surfaceScalarField;`. GeometricField needs `writeObject()` override (separate from Fix #50).

### What's Next

1. ~~**Test SSCP generic backend on Windows**~~ — **DONE** (2026-03-13). Generic backend builds 106 DLLs + 144 EXEs. OMP and CUDA JIT both work.
2. ~~**Add native CUDA backend for Windows**~~ — **BUILD DONE** (2026-03-26). 113 DLLs + 168 EXEs with `WM_COMPILER=Cuda`. blockMesh works. **Runtime issue**: simpleFoam segfaults during mesh creation when CUDA `lambdaKernel` executes. See below.
3. **Debug simpleFoam CUDA kernel crash** — segfault in `_backendFor` (cudaExecutor.cu:255) during "Create mesh for time = 0". The SYCL backend works at this point. The native CUDA backend (Clang `-x cuda`) crashes. This needs `compute-sanitizer` investigation.
4. Test other solvers once simpleFoam works
5. Fix GeometricField `writeObject()` override for correct `phi` class name (separate from Fix #50)
6. ~~Test AMD GPU support via `ACPP_TARGETS=hip:gfxXXX`~~ — **Replaced by native HIP backend** (`WM_COMPILER=Hip`). Compiles with AMD ROCm 7.1 Clang on Windows. Build in progress.

### Critical wmake Knowledge

1. **Variable passing**: GNU Make does NOT inherit env vars. wmake passes them as `VAR="$VAR"` pairs.
2. **sourceFiles generation**: Uses cpp+sed, does NOT expand Make variables. Use `#ifdef` in C++ instead.
3. **Windows system libs**: Use `-Wl,/DEFAULTLIB:libname` not bare `libname.lib` (clang++ fails on bare `.lib`).
4. **`.dep.part` workaround**: After header changes, run `find build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;` to fix stale dependencies.
5. **Static init order in NoRepository templates**: Never use `typeName` (dynamic init) in NoRepository template code that crosses DLL boundaries. Use `typeName_()`. RTS registration default arg is `typeName_()` (safe for cross-DLL), specific macros pass explicit names where needed (Fix #45).
6. **Template code = cross-DLL code**: `#ifdef NoRepository` templates in `src/OpenFOAM/` get compiled into downstream DLLs. Any `::typeName` in them is a cross-DLL data access. Always use `::typeName_()`.
7. **lnInclude staleness**: On Windows, lnInclude contains COPIES. After editing template/header files, MUST copy to lnInclude manually.
8. **Force recompilation**: wmake may not detect template header changes. Delete the `.o` file to force recompilation.
9. **dllimport ignored for template class statics**: Use explicit specialization declarations (FOAM_VECTORSPACE_EXTERN_DATA macro) - see Fix #27.
10. **FOAM_TYPENAME_EXPORT is always dllexport**: `debug` and `typeName` static members never get dllimport. NoRepository templates accessing them crash. Use `#ifdef _WIN32` / `if (false)` guards - see Fix #33.
11. **Failed LoadLibrary destroys RTS tables**: Windows unloads partially-loaded transitive deps on failure → static destructors call `construct(false)` → shared tables deleted. Fix #47 disables table cleanup on Windows (`RTS_TABLE_CLEANUP = false`).
12. **Partial libOpenFOAM rebuild requires full downstream rebuild**: COMDAT folding selects one template function body from multiple .o files. Recompiling ANY .o file can change which body wins. Downstream DLLs crash if they depend on the old function body. wmake does NOT detect this — must manually delete .o files and rebuild.
13. **Clang CUDA `-x cuda`**: ALL `.C` files need `-x cuda` (kernel code in headers via NoRepository). No `-fgpu-rdc` needed — per-TU fatbin registration works, BUT `cudart.lib` must be in `LINKLIBSO` (not `LIB_LIBS`) with `/INCLUDE:__cudaRegisterFatBinary` to force the import (Fix #52). Must use `lld-link` (`-fuse-ld=lld`). Suppress all warnings (`-w`) — device-pass `__declspec` warnings cause silent `.o` output failure.
14. **NVIDIA device math `__nv_*` declarations need `__device__`**: On nvc++ (Linux), `extern "C"` functions in `__CUDA_ARCH__` blocks are implicitly device. On Clang, they need explicit `__device__` annotation or you get `reference to __host__ function in __host__ __device__ function`.
15. **CUDA atomics need `#ifdef __CUDA_ARCH__` guards**: `atomicAdd`/`atomicCAS` etc. are device-only builtins. Wrap calls in `#ifdef __CUDA_ARCH__` and annotate functions with `FOAM_DEVICE`.

### Build Scripts

- `build-openfoam-lib.sh` - Full build: OSspecific → Pstream (3-pass) → libOpenFOAM
- `rebuild-openfoam-dll.sh` - Quick rebuild of just libOpenFOAM.dll
- `rebuild-fv.sh` - Rebuild downstream chain: fileFormats → surfMesh → meshTools → blockMesh → extrudeModel → finiteVolume → dynamicMesh → blockMesh app
- `rebuild-simplefoam-deps.sh` - Rebuild simpleFoam dependency chain
- `rebuild-fv-sf.sh` - Quick rebuild: finiteVolume + simpleFoam only
- `rebuild-fv-sf-fix27.sh` - Full rebuild: finiteVolume (wmake -k) + simpleFoam with Fix #27
- `rebuild-blockmesh-quick.sh` - Quick rebuild: blockMesh lib + app only

### Debug Traces

All debug traces have been removed from source files (15 files cleaned, ~375 lines removed).
