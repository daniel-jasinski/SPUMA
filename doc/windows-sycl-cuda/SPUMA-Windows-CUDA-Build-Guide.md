# SPUMA Windows MSVC+CUDA Build Guide

## Project Overview

**SPUMA** (Simulation Processing in Unified Memory Accelerators) is a GPU-accelerated CFD software based on OpenFOAM-v2412, developed by Cineca. It implements GPU porting of OpenFOAM targeting NVIDIA (CUDA), AMD (HIP), and Intel GPUs using a unified memory approach.

**Target Platform:** Windows with NVIDIA CUDA GPUs via SYCL/AdaptiveCpp

## Build Configuration

### Platform Identifier
- **Recommended:** `win64MsvcSyclDPInt32Opt` (32-bit labels)
- **Alternative:** `win64MsvcSyclDPInt64Opt` (64-bit labels for very large meshes)

### Toolchain Requirements
| Component | Version | Path |
|-----------|---------|------|
| LLVM/Clang | 21.x | `C:\dev\llvm-acpp-msvc-21` |
| AdaptiveCpp | Integrated with LLVM | Same as above |
| CUDA Toolkit | **12.5 or 12.6** (NOT 13.0+) | `C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5` |
| MSYS2 | UCRT64 | `C:\msys64` |

**CRITICAL:** CUDA 13.0+ is NOT supported because NVIDIA removed `texture_fetch_functions.h` which Clang depends on.

### Environment Variables (MSYS2 Bash)
```bash
export WM_PROJECT=OpenFOAM
export WM_PROJECT_VERSION=com
export WM_PROJECT_DIR=/c/OpenFOAM/SPUMA
export WM_DIR=$WM_PROJECT_DIR/wmake
export WM_OSTYPE=MSwindows
export WM_COMPILER=MsvcSycl
export WM_COMPILER_TYPE=system
export WM_PRECISION_OPTION=DP
export WM_LABEL_SIZE=32
export WM_COMPILE_OPTION=Opt
export WM_ARCH=win64
export WM_LABEL_OPTION=Int32
export WM_OPTIONS=win64MsvcSyclDPInt32Opt
export FOAM_LIBBIN=$WM_PROJECT_DIR/platforms/$WM_OPTIONS/lib
export FOAM_APPBIN=$WM_PROJECT_DIR/platforms/$WM_OPTIONS/bin
export LIB_SRC=$WM_PROJECT_DIR/src
export FOAM_MPI=dummy
export have_sycl=true
export ACPP_PATH=/c/dev/llvm-acpp-msvc-21
export ACPP_TARGETS=cuda:sm_86  # Adjust for your GPU
export CUDA_PATH_WIN="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5"
export PATH="$WM_PROJECT_DIR/wmake:$ACPP_PATH/bin:$PATH"
```

### Windows CMD Environment
```batch
@echo off
set WM_PROJECT_DIR=C:\OpenFOAM\SPUMA
set FOAM_LIBBIN=%WM_PROJECT_DIR%\platforms\win64MsvcSyclDPInt32Opt\lib
set FOAM_APPBIN=%WM_PROJECT_DIR%\platforms\win64MsvcSyclDPInt32Opt\bin
set ACPP_PATH=C:\dev\llvm-acpp-msvc-21
set CUDA_PATH=C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5
set PATH=%FOAM_APPBIN%;%FOAM_LIBBIN%;%ACPP_PATH%\bin;%CUDA_PATH%\bin;%PATH%
```

---

## Critical Code Changes for Windows DLL Compatibility

### Problem: Cross-DLL Static Data Access

Windows DLLs require explicit `__declspec(dllexport)` and `__declspec(dllimport)` annotations for static data members to be accessible across DLL boundaries. Without these, you get:
- **STATUS_ENTRYPOINT_NOT_FOUND (0xC0000139):** Symbol not exported from DLL
- **STATUS_ACCESS_VIOLATION (0xC0000005):** Symbol resolved incorrectly

### Solution: Export Macros in stdFoam.H

Three macros need to be added/modified in `src/OpenFOAM/include/stdFoam.H`:

#### 1. FOAM_EXPORT_DATA
For extern data symbols like `FatalError`, `Info`, `Warning`:
```cpp
#if defined(_WIN32)
# if defined(FOAM_BUILDING_LIB)
#  define FOAM_EXPORT_DATA __declspec(dllexport)
# else
#  define FOAM_EXPORT_DATA __declspec(dllimport)
# endif
#else
# define FOAM_EXPORT_DATA
#endif
```

#### 2. FOAM_TYPENAME_EXPORT
For TypeName/ClassName macro static members (`typeName`, `debug`):
```cpp
#if defined(_WIN32)
# define FOAM_TYPENAME_EXPORT __declspec(dllexport)
#else
# define FOAM_TYPENAME_EXPORT
#endif
```
**Note:** Always `dllexport` on Windows because declaration and definition must match.

#### 3. FOAM_DEFINE_STATIC_EXPORT
For static member definitions in .C files:
```cpp
#if defined(_WIN32)
# define FOAM_DEFINE_STATIC_EXPORT __declspec(dllexport)
#else
# define FOAM_DEFINE_STATIC_EXPORT
#endif
```

---

## Files Requiring Modification

### Core Header Changes

| File | Changes |
|------|---------|
| `src/OpenFOAM/include/stdFoam.H` | Add FOAM_EXPORT_DATA, FOAM_TYPENAME_EXPORT, FOAM_DEFINE_STATIC_EXPORT macros |
| `src/OpenFOAM/db/error/error.H` | `FOAM_EXPORT_DATA extern error FatalError;` etc. |
| `src/OpenFOAM/db/error/messageStream.H` | `FOAM_EXPORT_DATA extern messageStream Info;` etc. |
| `src/OpenFOAM/db/Time/Time.H` | `FOAM_EXPORT_DATA static word controlDictName;` |
| `src/OpenFOAM/meshes/polyMesh/polyMesh.H` | `FOAM_EXPORT_DATA static word defaultRegion;` |
| `src/OpenFOAM/db/IOstreams/IOstreams/IOstream.H` | `FOAM_EXPORT_DATA static unsigned int precision_;` |
| `src/OpenFOAM/db/IOstreams/Pstreams/UPstream.H` | Multiple static members exported |
| `src/OpenFOAM/primitives/strings/word/word.H` | `FOAM_TYPENAME_EXPORT static int debug;` |
| `src/OpenFOAM/containers/HashTables/HashTable/HashTableCore.H` | `FOAM_EXPORT_DATA static const label maxTableSize;` |
| `src/OpenFOAM/global/argList/argList.H` | All static members need FOAM_EXPORT_DATA |

### className.H Macro Changes

In `src/OpenFOAM/db/typeInfo/className.H`:
```cpp
// ClassNameNoDebug macro
#define ClassNameNoDebug(TypeNameString)                                       \
    FOAM_TYPENAME_EXPORT static const ::Foam::word typeName

// ClassName macro
#define ClassName(TypeNameString)                                              \
    ClassNameNoDebug(TypeNameString);                                          \
    FOAM_TYPENAME_EXPORT static int debug
```

### defineDebugSwitch.H Changes

In `src/OpenFOAM/global/debug/defineDebugSwitch.H`:
```cpp
// defineDebugSwitchWithName macro
#define defineDebugSwitchWithName(Type, Name, Value)                           \
    FOAM_DEFINE_STATIC_EXPORT int Type::debug(::Foam::debug::debugSwitch(Name, Value))

// defineTypeNameWithName macro (in className.H)
#define defineTypeNameWithName(Type, Name)                                     \
    FOAM_DEFINE_STATIC_EXPORT const ::Foam::word Type::typeName(Name)
```

### Runtime Selection Table Changes

See `Runtime-Selection-Table-Fix.md` for the function-based table access pattern needed to prevent table duplication across DLLs.

---

## HashTableCore Fix

**Problem:** Windows MSVC uses 32-bit `long`, causing undefined behavior with `1L << 61`.

**File:** `src/OpenFOAM/containers/HashTables/HashTable/HashTableCore.H`

```cpp
// Before (causes UB on Windows):
static const label maxTableSize = 1L << 61;

// After:
FOAM_EXPORT_DATA static const label maxTableSize;

// In HashTableCore.C:
const Foam::label Foam::HashTableCore::maxTableSize = Foam::label(1) << 61;
```

---

## argList Static Members Export

**Problem:** `argList::addNote()` and similar functions access static members that aren't exported.

**File:** `src/OpenFOAM/global/argList/argList.H`

Add `#include "stdFoam.H"` and annotate all static members:
```cpp
// Static Data Members
FOAM_EXPORT_DATA static SLList<string> validArgs;
FOAM_EXPORT_DATA static HashSet<string> advancedOptions;
FOAM_EXPORT_DATA static HashTable<string> validOptions;
FOAM_EXPORT_DATA static HashTable<string> validParOptions;
FOAM_EXPORT_DATA static HashTable<std::pair<word,int>> validOptionsCompat;
FOAM_EXPORT_DATA static HashTable<std::pair<bool,int>> ignoreOptionsCompat;
FOAM_EXPORT_DATA static HashTable<string, label, Hash<label>> argUsage;
FOAM_EXPORT_DATA static HashTable<string> optionUsage;
FOAM_EXPORT_DATA static SLList<string> notes;
FOAM_EXPORT_DATA static std::string::size_type usageMin;
FOAM_EXPORT_DATA static std::string::size_type usageMax;
FOAM_EXPORT_DATA static word postProcessOptionName;
```

---

## Build System Changes

### wmake Variable Passing

GNU Make does NOT inherit shell environment variables. The `wmake` script explicitly passes them as `VAR="$VAR"` pairs on the make command line. This was added in 3 locations in `wmake/wmake` for:
- `have_cuda`, `have_hip`, `have_sycl` (GPU backend flags)
- `WIN_TMP`, `WIN_CUDA_LIB`, `WIN_ACPP_LIB`, `WIN_MSVC_LIB`, `WIN_SDK_LIB`, `WIN_UCRT_LIB` (linker paths)

### sourceFiles Generation

The `sourceFiles` list is generated by cpp+sed in `wmake/makefiles/files`. It does NOT expand Make variables like `$(DEVICE_SOURCE)`. To add conditional source files, either:
- Add them directly to `Make/files` with `#ifdef` guards in the C++ code
- Or use cpp `#if` directives in Make/files

### Windows System Library Linking

When linking via acpp/clang++, **never** pass bare `.lib` filenames — clang++ tries to open them as files:
```makefile
# WRONG:  advapi32.lib       → clang++: error: no such file or directory
# RIGHT:  -Wl,/DEFAULTLIB:advapi32  → passed to link.exe which searches LIB paths
```

### Linker Flags

`/FORCE:UNRESOLVED` is set in `wmake/rules/win64MsvcSycl/c++` on both `LINKLIBSO` and `LINKEXE`. This allows linking to succeed despite unresolved Pstream/MPI symbols.

### DEF File Generation

`wmake/scripts/generate-msvc-def` extracts global symbols from object files using `llvm-nm` and generates a `.def` file for explicit symbol export. This is invoked automatically during `wmake libso`.

### WIN_* Link Variables

wmake computes MSVC library search paths using `cygpath -w -s` (8.3 short names to avoid spaces) and passes them to make. These populate the `LIB` env var for link.exe:
- `WIN_SDK_LIB` — Windows SDK (`advapi32.lib`, `kernel32.lib`, etc.)
- `WIN_MSVC_LIB` — MSVC runtime libraries
- `WIN_CUDA_LIB` — CUDA toolkit libraries
- `WIN_ACPP_LIB` — AdaptiveCpp libraries
- `WIN_UCRT_LIB` — Universal C Runtime

### Response Files

Windows command-line length limits require object file lists to be written to `.rsp` files (handled automatically by `wmake/makefiles/general`).

---

## Build Order (Dependency Order)

```bash
# Prerequisites (use build-openfoam-lib.sh for automated build)
cd src/OSspecific/MSwindows && wmake libo
cd src/Pstream/dummy && wmake libo        # Pass 1: static lib
cd src/OpenFOAM && FOAM_LINK_DUMMY_PSTREAM=libo wmake libso
cd src/Pstream/dummy && wmake libso       # Pass 2: DLL

# Core libraries
cd src/fileFormats && wmake libso
cd src/surfMesh && wmake libso
cd src/meshTools && wmake libso
cd src/finiteVolume && wmake libso

# Mesh utilities
cd src/mesh/extrudeModel && wmake libso
cd src/dynamicMesh && wmake libso
cd src/dynamicFvMesh && wmake libso

# Transport and turbulence
cd src/transportModels/twoPhaseMixture && wmake libso
cd src/transportModels/incompressible && wmake libso
cd src/TurbulenceModels/turbulenceModels && wmake libso
cd src/TurbulenceModels/incompressible && wmake libso

# Additional
cd src/sampling && wmake libso

# Solvers
cd applications/solvers/incompressible/simpleFoam && wmake
```

---

## Runtime Dependencies

Required DLLs in PATH:
- `acpp-rt.dll` - AdaptiveCpp runtime
- `acpp-common.dll` - AdaptiveCpp common library
- `cudart64_12.dll` - CUDA runtime

---

## Debugging Tips

### Check DLL Loading
```powershell
Add-Type -TypeDefinition @'
using System;
using System.Runtime.InteropServices;
public class NativeLib {
    [DllImport("kernel32.dll", SetLastError = true)]
    public static extern IntPtr LoadLibraryEx(string lpFileName, IntPtr hFile, uint dwFlags);
    [DllImport("kernel32.dll")]
    public static extern uint GetLastError();
}
'@

$handle = [NativeLib]::LoadLibraryEx("C:\path\to\lib.dll", [IntPtr]::Zero, 0)
if ($handle -eq [IntPtr]::Zero) {
    $err = [NativeLib]::GetLastError()
    Write-Host "FAILED: Error $err"
}
```

### Error Codes
| Code | Hex | Meaning |
|------|-----|---------|
| -1073741511 | 0xC0000139 | STATUS_ENTRYPOINT_NOT_FOUND - Missing DLL export |
| -1073741819 | 0xC0000005 | STATUS_ACCESS_VIOLATION - Bad memory access |
| 127 | 0x7F | ERROR_PROC_NOT_FOUND - Procedure not found in DLL |

---

## Key Insights

1. **Windows DLL Export Rules:**
   - Static data members MUST have matching dllexport/dllimport on declaration AND definition
   - You cannot add dllexport to a redeclaration that didn't have it
   - The DEF file generation handles function exports; data exports need manual annotation

2. **MSYS2 Path Handling:**
   - `/c/path` translates to `C:\path`
   - Be VERY careful with `rm -rf` - use Windows paths for safety
   - Prefer `cmd.exe //c "rmdir /s /q path"` for deletions
   - Use `cygpath -w -s` for 8.3 short paths (avoids spaces in CUDA/SDK paths)

3. **AdaptiveCpp/SYCL:**
   - Uses CUDA backend via `--acpp-targets=cuda:sm_XX`
   - Requires CUDA 12.5 or earlier (not 13.0+)
   - Runtime DLLs must be in PATH (`acpp-rt.dll`, `acpp-common.dll`)
   - Each `.C` file is compiled twice: host (MSVC ABI) + device (CUDA sm_XX)
   - `__declspec(dllexport)` warnings in device code are harmless

4. **OpenFOAM Static Initialization:**
   - IOstreams initialize first (debug output shows this)
   - argList static members accessed early in main()
   - Cross-DLL access requires proper export annotations

5. **wmake Build System:**
   - GNU Make does NOT inherit env vars — wmake passes them explicitly
   - `sourceFiles` generation uses cpp+sed, does NOT expand Make variables
   - Use `#ifdef` in C++ instead of Make conditionals for conditional source inclusion
   - Windows library linking: use `-Wl,/DEFAULTLIB:libname` not bare `libname.lib`

6. **Pstream Cyclic Dependency:**
   - libOpenFOAM and Pstream have a circular dependency
   - Resolved with 3-pass build: Pstream.lib → libOpenFOAM.dll → Pstream.dll
   - ~30 unresolved Pstream/MPI symbols expected with `/FORCE:UNRESOLVED`

7. **Build Performance:**
   - libOpenFOAM.dll: ~600 source files, ~2 hours on single-threaded build
   - Dual-pass compilation (host+CUDA) doubles compile time vs host-only
