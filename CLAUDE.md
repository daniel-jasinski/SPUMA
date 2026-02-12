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
```bash
WM_COMPILER=MsvcSycl
WM_LABEL_SIZE=32
export have_sycl=true
export ACPP_PATH="/c/dev/llvm-acpp-msvc-21"
export ACPP_TARGETS=cuda:sm_86
export CUDA_PATH_WIN="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5"
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

## Current Build Status (2026-02-12)

### What's Built

- ✅ `libOSspecific.lib` (1.7 MB static)
- ✅ `libPstream_static.lib` (812 KB, llvm-ar from .o files)
- ✅ `libPstream.dll` (dummy, serial-only, for downstream libraries)
- ✅ **`libOpenFOAM.dll`** (49 MB, ~600 source files, host + CUDA sm_86) - Loads successfully
- ✅ `libfileFormats.dll`, `libsurfMesh.dll`, `libmeshTools.dll`, `libblockMesh.dll`
- ✅ `libextrudeModel.dll`, `libfiniteVolume.dll`, `libdynamicMesh.dll`
- ✅ **`blockMesh.exe`** - Runs successfully on pitzDaily (12,225 cells generated)

### Fixes Applied (since last update)

- Fix #13: `nullObject.H` - Added `FOAM_EXPORT_DATA` on `nullObjectPtr` for cross-DLL iterator access
- Fix #14: `blockMeshTopology.C` - Changed `::typeName` to `::typeName_()` for cross-DLL safety
- Fix #15: `FlexLexer.h` copied to `src/fileFormats/lnInclude/` for CUDA device compilation pass
- Fix #16 (PENDING REBUILD): `GeometricBoundaryField.C` - Changed `emptyPolyPatch::typeName` and `cyclicPolyPatch::typeName` to `typeName_()` in template code instantiated by downstream DLLs
- `simplifiedDynamicFvMesh.H` - Changed `DynamicMeshType::typeName_.c_str()` to `DynamicMeshType::typeName_()`

### Current State

- **blockMesh works!** Successfully generates mesh for pitzDaily tutorial
- **simpleFoam**: All DLLs built and loaded. Crashes during "Reading field p" (volScalarField constructor)
- Fix #16 applied to source but `volFields.o` is stale (Feb 12 11:52, before fix). Must delete and rebuild.
- See `doc/windows-sycl-cuda/Current-Status-Next-Steps.md` for exact rebuild commands.

### What's Next

1. **Immediate**: Delete stale `volFields.o`, rebuild `libfiniteVolume.dll` + `simpleFoam.exe`, re-test
2. If crash persists: add traces inside GeometricField constructor, check for more `::typeName` in template code
3. Get simpleFoam running on CPU (OMP backend), then test CUDA

### Critical wmake Knowledge

1. **Variable passing**: GNU Make does NOT inherit env vars. wmake passes them as `VAR="$VAR"` pairs.
2. **sourceFiles generation**: Uses cpp+sed, does NOT expand Make variables. Use `#ifdef` in C++ instead.
3. **Windows system libs**: Use `-Wl,/DEFAULTLIB:libname` not bare `libname.lib` (clang++ fails on bare `.lib`).
4. **`.dep.part` workaround**: After header changes, run `find build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;` to fix stale dependencies.
5. **Static init order**: Never use `typeName` (dynamic init) as default arg in registration constructors. Use `typeName_()` (`const char*` literal).
6. **Template code = cross-DLL code**: `#ifdef NoRepository` templates in `src/OpenFOAM/` get compiled into downstream DLLs. Any `::typeName` in them is a cross-DLL data access. Always use `::typeName_()`.
7. **lnInclude staleness**: On Windows, lnInclude contains COPIES. After editing template/header files, MUST copy to lnInclude manually.
8. **Force recompilation**: wmake may not detect template header changes. Delete the `.o` file to force recompilation.

### Build Scripts

- `build-openfoam-lib.sh` - Full build: OSspecific → Pstream (3-pass) → libOpenFOAM
- `rebuild-openfoam-dll.sh` - Quick rebuild of just libOpenFOAM.dll
- `rebuild-fv.sh` - Rebuild downstream chain: fileFormats → surfMesh → meshTools → blockMesh → extrudeModel → finiteVolume → dynamicMesh → blockMesh app
- `rebuild-simplefoam-deps.sh` - Rebuild simpleFoam dependency chain
- `rebuild-fv-sf.sh` - Quick rebuild: finiteVolume + simpleFoam only
- `rebuild-blockmesh-quick.sh` - Quick rebuild: blockMesh lib + app only

### Debug Traces Still Present

Remove before committing:
- `src/OpenFOAM/include/createMemoryPool.H` - TRACE: createMemoryPool
- `src/OpenFOAM/include/createTime.H` - TRACE: createTime
- `applications/utilities/mesh/generation/blockMesh/blockMesh.C` - TRACE: blockMesh
- `applications/utilities/mesh/generation/blockMesh/findBlockMeshDict.H` - TRACE: findBlockMeshDict
- `src/mesh/blockMesh/blockMesh/blockMeshTopology.C` - TRACE:topo
- `applications/solvers/incompressible/simpleFoam/simpleFoam.C` - `#include <cstdio>`
- `applications/solvers/incompressible/simpleFoam/createFields.H` - TRACE:sf
