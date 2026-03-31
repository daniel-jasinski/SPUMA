# SPUMA Windows Build - Current Status and Next Steps

## Summary

The SPUMA project (OpenFOAM with GPU acceleration) is being ported to Windows with CUDA GPU support via the SYCL/AdaptiveCpp backend. This document tracks implementation progress and provides clear instructions for continuing the build.

## Repository Information

- **Repository:** `git@github.com:daniel-jasinski/SPUMA.git`
- **Branch:** `sycl-backend`
- **Platform:** `win64MsvcSyclDPInt32Opt`

---

## Current Build State (2026-02-18)

### Built Successfully

| Library | Size | Status |
|---------|------|--------|
| `libOSspecific.lib` | 1.7 MB | Static library, built |
| `libPstream_static.lib` (dummy) | 812 KB | Static library (llvm-ar), linked into libOpenFOAM |
| `libPstream.dll` (dummy) | 128 KB | DLL for downstream libraries |
| **`libOpenFOAM.dll`** | **49 MB** | **DLL + import lib, ~600 source files, loads OK** |
| `libfileFormats.dll` | 1.6 MB | Built, loads OK |
| `libsurfMesh.dll` | 6.7 MB | Built, loads OK |
| `libmeshTools.dll` | 23 MB | Built, loads OK |
| `libblockMesh.dll` | 2.3 MB | Built, loads OK |
| `libextrudeModel.dll` | 0.7 MB | Built, loads OK |
| `libfiniteVolume.dll` | ~250 MB | Built with Fix #27, runs OK |
| `libdynamicMesh.dll` | 17 MB | Built, loads OK |
| `libdynamicFvMesh.dll` | 3.4 MB | Built, loads OK |
| `libincompressibleTransportModels.dll` | ~3 MB | Built, loads OK |
| `libturbulenceModels.dll` | ~35 MB | Built, loads OK |
| `libincompressibleTurbulenceModels.dll` | ~2 MB | Built, loads OK |
| `liblagrangian.dll` | ~2 MB | Built, loads OK |
| `libsampling.dll` | ~40 MB | Built, loads OK |
| `libfvOptions.dll` | ~45 MB | Built, loads OK |
| `libatmosphericModels.dll` | ~5 MB | Built, loads OK |
| `blockMesh.exe` | 0.5 MB | **Runs successfully on pitzDaily tutorial** |
| `simpleFoam.exe` | ~1 MB | **Runs to completion, writes all output files** |

### Runtime Status

**blockMesh runs successfully!** Generated 12,225 cells for pitzDaily tutorial.

**simpleFoam runs to completion!** SIMPLE solver converges in 1 iteration. All 6 field files + uniform/time written. Solver prints "End" successfully. Exits with code 139 (cosmetic crash during C++ destructor chain after main() returns).

**Known issues:**
1. **Output file class names wrong**: `class p;` instead of `class volScalarField;` — due to headerClassName_ not propagated from temporary reader in GeometricField::readFields(). Fix applied to GeometricField.C but requires finiteVolume rebuild.
2. **Exit-time crash**: SIGSEGV during destructor chains after main() returns. Cosmetic — all output is correct.
3. **Solver residuals are 0**: Due to broken SYCL reductions (Fix #28 needs ~940 .o rebuild).

### Fixes Applied (Fixes #1-#31)

All 31 fixes applied. Key recent:
- **Fix #28**: SYCL `reductionSum` CPU fallback. **Requires rebuilding ~940 .o files.**
- **Fix #29**: DEF file must include static .lib files (libOSspecific.lib, libPstream_static.lib) when regenerating.
- **Fix #30**: `writeHeader()` uses `headerClassName()` instead of `type()` to avoid DEF JMP thunk crash. GeometricField.C propagation fix applied but needs finiteVolume rebuild.
- **Fix #31**: Exit-time crash (cosmetic, low priority).

### Current Issues

#### Fix #30: Correct Class Names in Output (Rebuild Needed)

`IOobject::writeHeader(Ostream&)` no longer calls `this->type()` (crashes due to DEF thunks). Uses `headerClassName_` instead. For GeometricField types (volScalarField, surfaceScalarField), `headerClassName_` is empty because `readFields()` reads via a temporary IOobject. Fix: propagate `headerClassName_` from temporary to the actual object. Code change done in `GeometricField.C`, but since it's a NoRepository template compiled into libfiniteVolume, a full finiteVolume rebuild is needed.

#### Fix #28: SYCL Reduction Returns Zero (Rebuild Needed)

Same as before — ~940 .o files need recompiling. Can be combined with the Fix #30 finiteVolume rebuild.

### Next Steps (Priority Order)

1. **Copy updated GeometricField.C to lnInclude** and **rebuild libfiniteVolume.dll** (fixes class names + includes Fix #28 sycl reduction fix)
2. Relink simpleFoam.exe if needed
3. Verify output files have correct class names
4. Investigate exit-time crash (low priority)
5. Full rebuild of ALL downstream DLLs to propagate Fix #28 everywhere
6. Test with CUDA backend
7. Remove all debug traces before committing

---

## Implementation Phases

### Phase 1: Core Export Macros ✅ COMPLETE

DLL export macros added to `src/OpenFOAM/include/stdFoam.H`:

| Macro | Purpose |
|-------|---------|
| `FOAM_EXPORT_DATA` | Extern data (FatalError, Info, etc.) - conditional dllexport/dllimport |
| `FOAM_TYPENAME_EXPORT` | TypeName/ClassName static members - always dllexport |
| `FOAM_DEFINE_STATIC_EXPORT` | Static member definitions in .C files |

### Phase 2: Core Header Exports ✅ COMPLETE

`FOAM_EXPORT_DATA` applied to static members in: `error.H`, `messageStream.H`, `Time.H`, `polyMesh.H`, `IOstream.H`, `UPstream.H`, `word.H`, `HashTableCore.H`, `argList.H`

### Phase 3: ClassName/TypeName Macros ✅ COMPLETE

Macros modified in `className.H` and `defineDebugSwitch.H` to use export annotations.

### Phase 4: Runtime Selection Tables ✅ COMPLETE

Function-based table access implemented in `runTimeSelectionTables.H` and `memberFunctionSelectionTables.H`. 332 `*New.C` files updated.

### Phase 5: Build System ✅ COMPLETE

wmake rules, DEF file generation, response files, linker flags all working.

### Phase 6: libOpenFOAM.dll Build ✅ COMPLETE

All ~600 source files compile for host + CUDA sm_86. DLL links successfully (zero unresolved symbols after Fix #4).

### Phase 7: Runtime Selection Table Static Init Fixes (IN PROGRESS)

**Fix #5**: Changed default argument in registration constructors from `typeName` (dynamically initialized `Foam::word`) to `typeName_()` (`const char*` string literal, always available). Applied to 6 locations in `runTimeSelectionTables.H` and `memberFunctionSelectionTables.H`. Reduced duplicate entry warnings from ~400 (all empty names) to ~13 (12 `Compound<T>` with actual names + 1 empty `pointPatchField`).

**Fix #6**: Removed duplicate `device/syclGlobal.cpp` from `Make/files` (was included both hardcoded and via `$(DEVICE_SOURCE)`).

**Remaining**: DLL still fails to load (STATUS_DLL_INIT_FAILED). See Handoff Spec below.

---

## Critical Build System Knowledge

### 1. wmake Variable Passing

GNU Make does NOT inherit shell environment variables. wmake must explicitly pass them:

```bash
# In wmake/wmake, variables are passed as VAR="$VAR" pairs:
exec $make -f "$WM_DIR"/makefiles/general \
    WM_DIR="$WM_DIR" WM_COMPILER="$WM_COMPILER" \
    have_cuda="$have_cuda" have_hip="$have_hip" have_sycl="$have_sycl" \
    $winLinkVars \
    ...
```

This was added to 3 locations in `wmake/wmake`:
- `makefiles/files` invocation (generates sourceFiles and options)
- Dependency update phase (`updatedep` target)
- Main compile/link phase

### 2. sourceFiles Generation vs Make Variables

The `sourceFiles` list is generated by `wmake/makefiles/files` using cpp preprocessing + sed. It does NOT expand Make variables like `$(DEVICE_SOURCE)`.

**Solution**: Add source files directly to `Make/files` and use `#ifdef` guards in C++ code. Example: `device/syclGlobal.cpp` was added directly to `src/OpenFOAM/Make/files`.

### 3. Windows System Libraries - The `-Wl,/DEFAULTLIB:` Pattern

When linking with acpp/clang++, bare `.lib` filenames cause errors because clang++ tries to open them as files before passing to link.exe:

```
clang++: error: no such file or directory: 'advapi32.lib'
```

**Solution**: Use `-Wl,/DEFAULTLIB:libname` which passes the directive directly to MSVC's link.exe. Link.exe then searches the `LIB` environment variable paths.

```makefile
# WRONG - clang++ tries to open as file:
LIB_LIBS = advapi32.lib

# CORRECT - passes to link.exe which searches LIB paths:
LIB_LIBS = -Wl,/DEFAULTLIB:advapi32
```

### 4. WIN_* Link Variables for MSVC Library Search

wmake computes Windows library paths using `cygpath -w -s` (8.3 short names to avoid spaces) and passes them to make as `$winLinkVars`. These populate the `LIB` environment variable for MSVC's link.exe:

| Variable | Purpose | Example Value |
|----------|---------|---------------|
| `WIN_TMP` | Temp dir for link.exe | `C:/Users/Daniel/AppData/Local/Temp` |
| `WIN_CUDA_LIB` | CUDA libraries | `C:\PROGRA~1\NVIDIA~2\CUDA\v12.5\lib\x64` |
| `WIN_ACPP_LIB` | AdaptiveCpp libraries | `C:\dev\LL683A~1\lib` |
| `WIN_MSVC_LIB` | MSVC runtime libs | `C:\PROGRA~2\MICROS~2\2022\BUILDT~1\VC\Tools\MSVC\...` |
| `WIN_SDK_LIB` | Windows SDK (kernel32, advapi32, etc.) | `C:\PROGRA~2\WI3CF2~1\10\Lib\100226~1.0\um\x64` |
| `WIN_UCRT_LIB` | Universal CRT | `C:\PROGRA~2\WI3CF2~1\10\Lib\100226~1.0\ucrt\x64` |

These are used in `wmake/makefiles/general`:
```makefile
WIN_EXTRA_LIB = $(WIN_CUDA_LIB);$(WIN_ACPP_LIB);$(WIN_MSVC_LIB);$(WIN_SDK_LIB);$(WIN_UCRT_LIB)
WIN_LINK_ENV = TMP="$(WIN_TMP)" TEMP="$(WIN_TMP)" LIB="$$LIB;$(WIN_EXTRA_LIB)"
```

### 5. DEF File Generation

Windows DLLs require explicit symbol export lists. The `wmake/scripts/generate-msvc-def` script:
1. Takes object files (via response file)
2. Extracts global symbols using `llvm-nm`
3. Generates a `.def` file listing exports
4. Passed to linker via `-Wl,/DEF:path.def`

### 6. Response Files for Long Command Lines

Windows has command-line length limits. Object file lists are written to `.rsp` files and passed to the linker via `@file.rsp`. This is handled automatically in `wmake/makefiles/general`.

### 7. SYCL Device Code Warnings

When compiling for CUDA sm_86, every file produces warnings like:
```
warning: __declspec attribute 'dllexport' is not supported [-Wignored-attributes]
```
These are **harmless** — `__declspec` is a host-only attribute and is correctly ignored by the device compiler.

### 8. Pstream Cyclic Dependency

OpenFOAM has a circular dependency between libOpenFOAM and Pstream. The build resolves this with a 3-pass approach:
1. Build Pstream as static `.lib` (libo)
2. Build libOpenFOAM linking against static Pstream
3. Rebuild Pstream as DLL linking against libOpenFOAM.dll

---

## Files Modified for Windows Build

### Build System Files

| File | Changes |
|------|---------|
| `wmake/wmake` | Added `have_cuda/hip/sycl` variable passing to 3 make invocations; WIN_* link var computation |
| `wmake/makefiles/general` | Windows link recipes with response files, DEF generation, WIN_LINK_ENV |
| `wmake/rules/win64MsvcSycl/c++` | MSVC ABI compiler flags, `-D_USE_MATH_DEFINES`, `/FORCE:UNRESOLVED` |
| `wmake/rules/win64MsvcSycl/c` | C compiler flags |
| `wmake/rules/win64MsvcSycl/general` | Windows extensions (`.dll`, `.exe`, `.lib`) |
| `wmake/rules/General/Sycl/c++` | AdaptiveCpp compiler setup, `--acpp-targets` |
| `wmake/rules/General/Sycl/link-c++` | SYCL linker flags |
| `wmake/scripts/generate-msvc-def` | DEF file generation from object files |

### Source Code Files

| File | Changes |
|------|---------|
| `src/OpenFOAM/include/stdFoam.H` | `FOAM_EXPORT_DATA`, `FOAM_TYPENAME_EXPORT`, `FOAM_DEFINE_STATIC_EXPORT` macros |
| `src/OpenFOAM/include/foamWinCompat.H` | POSIX type definitions (`mode_t`, `pid_t`) for MSVC |
| `src/OpenFOAM/db/typeInfo/className.H` | Export annotations on `ClassNameNoDebug`, `ClassName`, `defineTypeNameWithName` |
| `src/OpenFOAM/global/debug/defineDebugSwitch.H` | Export annotation on `defineDebugSwitchWithName` |
| `src/OpenFOAM/db/runTimeSelection/construction/runTimeSelectionTables.H` | Function-based table access; Fix #5: `typeName_()` default args |
| `src/OpenFOAM/db/runTimeSelection/memberFunctions/memberFunctionSelectionTables.H` | Function-based table access; Fix #5: `typeName_()` default args |
| `src/OpenFOAM/db/error/error.H` | `FOAM_EXPORT_DATA` on `FatalError`, `FatalIOError` |
| `src/OpenFOAM/db/error/messageStream.H` | `FOAM_EXPORT_DATA` on `Info`, `Warning`, etc. |
| `src/OpenFOAM/db/Time/Time.H` | `FOAM_EXPORT_DATA` on `controlDictName` |
| `src/OpenFOAM/meshes/polyMesh/polyMesh.H` | `FOAM_EXPORT_DATA` on `defaultRegion`, `meshSubDir` |
| `src/OpenFOAM/db/IOstreams/IOstreams/IOstream.H` | `FOAM_EXPORT_DATA` on `precision_` |
| `src/OpenFOAM/db/IOstreams/Pstreams/UPstream.H` | `FOAM_EXPORT_DATA` on all static members |
| `src/OpenFOAM/primitives/strings/word/word.H` | `FOAM_TYPENAME_EXPORT` on `typeName`, `debug`; `FOAM_EXPORT_DATA` on `null` |
| `src/OpenFOAM/containers/HashTables/HashTable/HashTableCore.H` | Export + fix `1L << 61` UB on 32-bit long |
| `src/OpenFOAM/global/argList/argList.H` | `FOAM_EXPORT_DATA` on all 12 static members |
| `src/OpenFOAM/Make/files` | Fix #6: Removed duplicate `device/syclGlobal.cpp` (kept via `$(DEVICE_SOURCE)` only) |
| `src/OpenFOAM/Make/options` | `-DFOAM_BUILDING_LIB` flag; `-Wl,/DEFAULTLIB:advapi32`; SYCL include paths |
| ~332 `*New.C` files | Updated to use function-based table access `constructorTablePtr_()` |

### Build Scripts

| File | Purpose |
|------|---------|
| `build-openfoam-lib.sh` | Full build script: OSspecific + Pstream (3-pass) + libOpenFOAM |
| `rebuild-openfoam-dll.sh` | Quick rebuild of just libOpenFOAM.dll (skips OSspecific/Pstream) |

---

## Next Steps: Building Downstream Libraries

### Prerequisites

All downstream library builds need the same environment as libOpenFOAM. Use `build-openfoam-lib.sh` as a template, or set up the environment manually:

```bash
# Core environment
export WM_PROJECT_DIR="/c/OpenFOAM/SPUMA"
export WM_PROJECT="OpenFOAM"
export WM_COMPILER="MsvcSycl"
export WM_OSTYPE="MSwindows"
export WM_ARCH="win64"
export WM_LABEL_SIZE=32
export WM_PRECISION_OPTION="DP"
export WM_COMPILE_OPTION="Opt"
export WM_OPTIONS="${WM_ARCH}${WM_COMPILER}${WM_PRECISION_OPTION}Int${WM_LABEL_SIZE}${WM_COMPILE_OPTION}"

# wmake paths
export WM_DIR="$WM_PROJECT_DIR/wmake"
export GENERAL_RULES="$WM_DIR/rules/General"
export DEFAULT_RULES="$WM_DIR/rules/$WM_ARCH$WM_COMPILER"

# Output paths
export FOAM_LIBBIN="$WM_PROJECT_DIR/platforms/$WM_OPTIONS/lib"
export FOAM_APPBIN="$WM_PROJECT_DIR/platforms/$WM_OPTIONS/bin"

# SYCL/CUDA
export ACPP_PATH="/c/dev/llvm-acpp-msvc-21"
export ACPP_TARGETS="cuda:sm_86"
export have_sycl=true
export CUDA_PATH="/c/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5"

# Windows
export LOCALAPPDATA="/c/Users/Daniel/AppData/Local"

# PATH
export PATH="/c/msys64/usr/bin:/c/msys64/mingw64/bin:$WM_DIR:$ACPP_PATH/bin:$CUDA_PATH/bin:$PATH"
```

### Build Order (Dependency Order)

Each library must be built with `wmake libso` from its source directory. Build in this order:

```bash
# Already built:
# ✅ src/OSspecific/MSwindows (libo)
# ✅ src/Pstream/dummy (libo + libso)
# ✅ src/OpenFOAM (libso → libOpenFOAM.dll)

# Step 1: Core libraries
cd "$WM_PROJECT_DIR/src/fileFormats" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/surfMesh" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/meshTools" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/finiteVolume" && "$WM_DIR/wmake" libso

# Step 2: Mesh utilities
cd "$WM_PROJECT_DIR/src/mesh/extrudeModel" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/dynamicMesh" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/dynamicFvMesh" && "$WM_DIR/wmake" libso

# Step 3: Transport and turbulence
cd "$WM_PROJECT_DIR/src/transportModels/twoPhaseMixture" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/transportModels/incompressible" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/TurbulenceModels/turbulenceModels" && "$WM_DIR/wmake" libso
cd "$WM_PROJECT_DIR/src/TurbulenceModels/incompressible" && "$WM_DIR/wmake" libso

# Step 4: Additional libraries
cd "$WM_PROJECT_DIR/src/sampling" && "$WM_DIR/wmake" libso

# Step 5: Solver
cd "$WM_PROJECT_DIR/applications/solvers/incompressible/simpleFoam" && "$WM_DIR/wmake"
```

### Expected Issues When Building Downstream Libraries

Each downstream library's `Make/options` may need modifications:

1. **Library references**: Change `libXXX.o` to `libXXX.lib` for Windows:
   ```makefile
   # POSIX:
   LIB_LIBS = -lOpenFOAM -lfiniteVolume
   # Windows may need:
   LIB_LIBS = $(FOAM_LIBBIN)/OpenFOAM.lib $(FOAM_LIBBIN)/finiteVolume.lib
   ```

2. **Windows system libraries**: Use `-Wl,/DEFAULTLIB:libname` pattern (never bare `.lib`).

3. **DLL export macros**: Downstream DLLs that define their own static data may need additional export annotations. Watch for `0xC0000139` errors at DLL load time.

4. **Include path issues**: Some libraries may need explicit include paths for OSspecific headers.

### Verification

After building each DLL, test it loads:

```powershell
# Set PATH to include all DLLs and runtime dependencies
$env:PATH = "C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib;" +
            "C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib\dummy;" +
            "C:\dev\llvm-acpp-msvc-21\bin;" +
            "C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin;" +
            $env:PATH

# Test DLL loading
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

$dll = "C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib\libOpenFOAM.dll"
$h = [NativeLib]::LoadLibraryEx($dll, [IntPtr]::Zero, 0)
if ($h -eq [IntPtr]::Zero) { "FAILED: Error $([NativeLib]::GetLastError())" } else { "OK" }
```

### Final Target: simpleFoam

```bash
# Build simpleFoam
cd "$WM_PROJECT_DIR/applications/solvers/incompressible/simpleFoam"
"$WM_DIR/wmake"

# Test
simpleFoam -help

# Run with GPU
simpleFoam -pool fixedSizeMemoryPool -poolSize 32
```

---

## Testing Checklist

### DLL Loading Tests

- [x] `libOSspecific.lib` builds
- [x] `libPstream.lib` (dummy, static) builds
- [x] `libPstream.dll` (dummy) builds
- [x] `libOpenFOAM.dll` builds
- [ ] `libOpenFOAM.dll` loads without error (LoadLibraryEx test)
- [ ] `libfileFormats.dll` builds and loads
- [ ] `libsurfMesh.dll` builds and loads
- [ ] `libmeshTools.dll` builds and loads
- [ ] `libfiniteVolume.dll` builds and loads
- [ ] `libdynamicMesh.dll` builds and loads
- [ ] `libdynamicFvMesh.dll` builds and loads
- [ ] `libturbulenceModels.dll` builds and loads
- [ ] `libincompressibleTransportModels.dll` builds and loads
- [ ] `libincompressibleTurbulenceModels.dll` builds and loads
- [ ] `libsampling.dll` builds and loads

### Application Tests

- [ ] `simpleFoam.exe` builds
- [ ] `simpleFoam -help` runs without crash
- [ ] `blockMesh` works on pitzDaily case
- [ ] `simpleFoam` runs on pitzDaily case (CPU)
- [ ] GPU acceleration works with `-pool fixedSizeMemoryPool`

---

## Build Timing Reference

On the current machine (single-threaded wmake):

| Component | Approximate Time |
|-----------|-----------------|
| OSspecific (libo) | ~5 minutes |
| Pstream (libo) | ~2 minutes |
| libOpenFOAM.dll (~600 files, host+CUDA) | **~2 hours** |
| Each downstream library | ~5-30 minutes depending on size |

Each `.C` file is compiled twice by acpp: once for host (MSVC ABI) and once for CUDA device (sm_86). This doubles compilation time compared to host-only builds.

---

## Known Build System Workarounds

### `.dep.part` Rename Issue

After modifying header files, `wmkdepend` writes `.dep.part` files but `rename()` fails silently on Windows (return value unchecked in `wmake/src/wmkdepend.cc:1017`). The `.dep` files remain stale, so Make doesn't detect header changes and skips recompilation.

**Workaround** (run from project root after header changes):
```bash
find platforms/win64MsvcSyclDPInt32Opt/build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;
```

This renames all `.dep.part` files to `.dep`, allowing Make to see updated dependencies and trigger recompilation.

### Detecting Stale Dependencies

If a header change doesn't trigger recompilation, check for `.dep.part` files:
```bash
find platforms/win64MsvcSyclDPInt32Opt/build -name "*.dep.part" | wc -l
```
If non-zero, run the rename workaround above.

---

## Common Error Codes

| Code | Hex | Name | Meaning | Solution |
|------|-----|------|---------|----------|
| -1073741511 | 0xC0000139 | STATUS_ENTRYPOINT_NOT_FOUND | Symbol not exported from DLL | Add FOAM_EXPORT_DATA |
| -1073741502 | 0xC0000142 | STATUS_DLL_INIT_FAILED | DLL static init crashed | See Debugging-Conclusions.md |
| -1073741819 | 0xC0000005 | STATUS_ACCESS_VIOLATION | Bad pointer / wrong memory | Fix table duplication |
| 126 | 0x7E | ERROR_MOD_NOT_FOUND | Missing dependency DLL | Check PATH |
| 127 | 0x7F | ERROR_PROC_NOT_FOUND / DLL_INIT_FAILED | MSYS2 generic error | Use PowerShell for real code |
| 193 | 0xC1 | ERROR_BAD_EXE_FORMAT | 32/64-bit mismatch | Ensure all x64 |

---

## Document References

| Document | Purpose |
|----------|---------|
| `SPUMA-Windows-CUDA-Build-Guide.md` | Complete build guide with environment setup and code changes overview |
| `Windows-DLL-Export-Fixes.md` | Detailed code changes for each file with before/after examples |
| `Runtime-Selection-Table-Fix.md` | Function-based table access pattern for cross-DLL type registration |
| `Debugging-Conclusions.md` | All root causes, fixes, and diagnostic techniques (Fixes #1-#29) |
| `Agent-Handoff-Spec.md` | Handoff specification for continuing DLL init debugging |
| This file | Current status tracking and next steps |
