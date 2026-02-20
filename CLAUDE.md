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

## Current Build Status (2026-02-20)

### What's Built

- ✅ `libOSspecific.lib` (1.7 MB static)
- ✅ `libPstream_static.lib` (812 KB, llvm-ar from .o files)
- ✅ `libPstream.dll` (dummy, serial-only, for downstream libraries)
- ✅ **`libOpenFOAM.dll`** (42 MB, ~612 .o files, host + CUDA sm_86) - Loads successfully
- ✅ `libfileFormats.dll`, `libsurfMesh.dll`, `libmeshTools.dll`, `libblockMesh.dll`
- ✅ `libextrudeModel.dll`, `libdynamicMesh.dll`, `libdynamicFvMesh.dll`
- ✅ `libincompressibleTransportModels.dll`, `libturbulenceModels.dll`, `libincompressibleTurbulenceModels.dll`
- ✅ `libsampling.dll`, `libfvOptions.dll`, `libatmosphericModels.dll`
- ✅ **`blockMesh.exe`** - Runs successfully on pitzDaily (12,225 cells, exit 0)
- ✅ `libfiniteVolume.dll` (~248 MB, 428 .o files) - Built, loads OK
- ✅ **`simpleFoam.exe`** - **Runs to completion on pitzDaily, exit 0** (meshPtr.release() workaround)

### Fixes Applied (33 total)

All 33 fixes documented in `doc/windows-sycl-cuda/Debugging-Conclusions.md`. Key recent:
- Fix #29: DEF generation must include static .lib files (libOSspecific.lib, libPstream_static.lib)
- Fix #30: `writeHeader()` headerClassName check avoids `type()` for template types
- Fix #31: `std::_Exit(0)` workaround (superseded by Fix #33's targeted `meshPtr.release()`)
- Fix #32: RTTI-based writeHeader returns OpenFOAM typeName (not typedef names). Fixes restart.
- Fix #33: `meshObject::debug` cross-DLL access in NoRepository template crashes mesh destructor. `MESHOBJECT_DEBUG` macro in MeshObject.C + `meshPtr.release()` in solver.

### Current State

- **blockMesh works!** Exit 0.
- **simpleFoam works!** Exit 0 (with `meshPtr.release()` workaround for Fix #33).
- **Restart works!** Exit 0.
- **Exit-time crash root cause identified**: `meshObject::debug` (cross-DLL data via DEF thunk) in NoRepository template MeshObject.C. Fix applied to source; requires libfiniteVolume.dll rebuild to fully eliminate mesh leak workaround.

### What's Next

1. Rebuild libfiniteVolume.dll with Fix #33 (MESHOBJECT_DEBUG macro) to eliminate mesh leak workaround
2. Test with CUDA backend (currently using OMP via ACPP_VISIBILITY_MASK=omp)
3. Full rebuild of ALL downstream DLLs for Fix #28 (SYCL reduction fix)
4. Remove remaining debug traces from DLLs (require full rebuild to take effect)
5. Check for other NoRepository templates with similar `if (debug)` patterns

### Critical wmake Knowledge

1. **Variable passing**: GNU Make does NOT inherit env vars. wmake passes them as `VAR="$VAR"` pairs.
2. **sourceFiles generation**: Uses cpp+sed, does NOT expand Make variables. Use `#ifdef` in C++ instead.
3. **Windows system libs**: Use `-Wl,/DEFAULTLIB:libname` not bare `libname.lib` (clang++ fails on bare `.lib`).
4. **`.dep.part` workaround**: After header changes, run `find build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;` to fix stale dependencies.
5. **Static init order**: Never use `typeName` (dynamic init) as default arg in registration constructors. Use `typeName_()` (`const char*` literal).
6. **Template code = cross-DLL code**: `#ifdef NoRepository` templates in `src/OpenFOAM/` get compiled into downstream DLLs. Any `::typeName` in them is a cross-DLL data access. Always use `::typeName_()`.
7. **lnInclude staleness**: On Windows, lnInclude contains COPIES. After editing template/header files, MUST copy to lnInclude manually.
8. **Force recompilation**: wmake may not detect template header changes. Delete the `.o` file to force recompilation.
9. **dllimport ignored for template class statics**: Use explicit specialization declarations (FOAM_VECTORSPACE_EXTERN_DATA macro) - see Fix #27.
10. **FOAM_TYPENAME_EXPORT is always dllexport**: `debug` and `typeName` static members never get dllimport. NoRepository templates accessing them crash. Use `#ifdef _WIN32` / `if (false)` guards - see Fix #33.

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
