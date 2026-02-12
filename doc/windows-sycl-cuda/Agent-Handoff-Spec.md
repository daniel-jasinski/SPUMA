# Agent Handoff Specification

**Date**: 2026-02-10
**Status**: DLL builds but fails to load at runtime (STATUS_DLL_INIT_FAILED)

---

## Required Reading Before Starting

1. **`doc/windows-sycl-cuda/Debugging-Conclusions.md`** - All 6 fixes applied so far with root causes and lessons learned
2. **`doc/windows-sycl-cuda/Current-Status-Next-Steps.md`** - Build system knowledge, file inventory, environment setup
3. **`CLAUDE.md`** - Safety rules and project overview

---

## Current Problem

`blockMesh.exe` exits with code 127 (STATUS_DLL_INIT_FAILED = 0xC0000142) when loading `libOpenFOAM.dll`. The DLL's static initialization gets far (globals, controlDict, debug switches, memory pool all initialize successfully) but then fails during runtime selection table registration.

### Latest Test Output (stderr_test5.txt)

```
TRACE: globals.C init start
debug::switchSet("InfoSwitches")
debug::controlDict() loading...
debug::controlDict() loaded OK
debug::switchSet("DebugSwitches")
debug::switchSet("OptimisationSwitches")
debug::switchSet("DimensionedConstants")
debug::switchSet("DimensionSets")
getInstance: creating dummyMemoryPool
getInstance: dummyMemoryPool created OK
TRACE: globals.C init done
Duplicate entry Compound<T> in runtime table compound    (x12)
Duplicate entry  in runtime table pointPatchField         (x1, empty name)
```

stdout is empty. Exit code 127.

### What "Duplicate entry" Means

These warnings come from registration constructors in `runTimeSelectionTables.H` (line 221) and `memberFunctionSelectionTables.H` (line 69). When `HashTable::insert()` fails (key already exists), the warning is printed. The table name (e.g., "compound", "pointPatchField") and the type name being registered are shown.

---

## Remaining Issues to Investigate

### Issue A: "Duplicate entry Compound<T>" (12 occurrences)

`Compound<T>` is a template class. Multiple template instantiations (e.g., `Compound<List<scalar>>`, `Compound<List<vector>>`, etc.) all register in the "compound" runtime table using `Compound<T>` as the type name. This is by design in OpenFOAM -- but on Linux these template instantiations are COMDAT-folded by the linker, so only one registration constructor survives. On Windows/MSVC, COMDAT folding may behave differently, resulting in duplicate registrations.

**Investigation directions**:
1. Check if these are actually harmful or just warnings. The `insert()` failure means later instantiations just silently skip. Does the process crash because of this or despite it?
2. Compare with Linux behavior -- does OpenFOAM on Linux also produce these warnings? If so, they may be benign.
3. Search for where `Compound<T>` is registered: `grep -r "addCompound" src/OpenFOAM/`

### Issue B: Empty name in pointPatchField table (1 occurrence)

One registration constructor is using an empty string as the type name for the "pointPatchField" table. Fix #5 changed the default argument from `typeName` to `typeName_()` in the standard macros, but this one entry is still empty. Possible reasons:
1. It uses a different registration mechanism not covered by Fix #5
2. It's a template specialization that doesn't go through the standard macros
3. It's registered via an explicit constructor call with a hardcoded empty/null name

**Investigation directions**:
1. Search for pointPatchField registration: `grep -rn "addToRunTimeSelectionTable.*pointPatchField" src/OpenFOAM/`
2. Search for `pointPatchFieldNew.C` -- this was already modified (listed in git status)
3. Check `src/OpenFOAM/fields/pointPatchFields/pointPatchField/` for unusual registration patterns
4. Add more detailed tracing to the registration constructor to print the table name AND the key being inserted (both the `Compound<T>` and the empty one)

### Issue C: STATUS_DLL_INIT_FAILED Root Cause

The duplicate entry warnings themselves may not be the cause of the DLL init failure. The actual crash/failure may happen after the last visible warning. Possibilities:
1. An exception is thrown during static init (C++ exceptions during DllMain are fatal on Windows)
2. A subsequent registration constructor dereferences a null pointer
3. The `safePrintStack` call after the duplicate warning causes issues (though it's a no-op on Windows)
4. Something else entirely fails after the last trace output

**Investigation directions**:
1. Add `std::cerr` trace output to more places in static initialization to narrow down where the failure occurs
2. Use PowerShell `Start-Process` to get the actual NTSTATUS code (not the truncated bash exit code):
   ```powershell
   $env:PATH = "C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib;C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib\dummy;C:\dev\llvm-acpp-msvc-21\bin;C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin;$env:PATH"
   $env:WM_PROJECT_DIR = "C:\OpenFOAM\SPUMA"
   $p = Start-Process -FilePath "C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\bin\blockMesh.exe" -Wait -PassThru -RedirectStandardError stderr.txt -RedirectStandardOutput stdout.txt
   "Exit code: $($p.ExitCode) (0x{0:X8})" -f [uint32]$p.ExitCode
   ```
3. Try `LoadLibraryEx` from PowerShell to load just the DLL without running blockMesh -- if the DLL itself fails to load, it's a static init issue; if it loads but blockMesh crashes in `main()`, it's a runtime issue
4. Use Windows Event Viewer (Application log) for crash details
5. Try running under `cdb.exe` (Windows Debugger) if available:
   ```
   cdb -g -G blockMesh.exe
   ```

---

## Build/Rebuild Instructions

### Full Rebuild After Code Changes

```bash
# 1. Source the environment
cd /c/OpenFOAM/SPUMA
source etc/bashrc

# 2. If headers were changed, fix stale .dep files
find platforms/win64MsvcSyclDPInt32Opt/build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;

# 3. Rebuild (takes ~2 hours for full libOpenFOAM)
bash rebuild-openfoam-dll.sh 2>&1 | tee build_log.txt

# 4. Test
export PATH="/c/OpenFOAM/SPUMA/platforms/win64MsvcSyclDPInt32Opt/lib:/c/OpenFOAM/SPUMA/platforms/win64MsvcSyclDPInt32Opt/lib/dummy:/c/dev/llvm-acpp-msvc-21/bin:$PATH"
blockMesh 2>stderr.txt; echo "EXIT=$?"
```

### Quick Test Without Full Rebuild

If you only changed a few `.C` files (not headers), `wmake` will only recompile those files:
```bash
cd /c/OpenFOAM/SPUMA/src/OpenFOAM
../../wmake/wmake libso 2>&1 | tee build_log.txt
```

### Adding Diagnostic Traces

To add `std::cerr` traces for debugging:
1. Add `#include <iostream>` if not already present
2. Use `std::cerr << "TRACE: description" << std::endl;`
3. Do NOT use `Info <<` or `FatalError` during static init -- these depend on objects that may not be initialized yet
4. Rebuild the affected file(s) with wmake

---

## Key Files for Investigation

| File | Purpose |
|------|---------|
| `src/OpenFOAM/db/runTimeSelection/construction/runTimeSelectionTables.H` | Core RTS macros with registration constructors (warnings at line 221) |
| `src/OpenFOAM/db/runTimeSelection/memberFunctions/memberFunctionSelectionTables.H` | Member function RTS macros (warnings at line 69) |
| `src/OpenFOAM/db/typeInfo/className.H` | Defines `typeName` (static word) and `typeName_()` (static const char*) |
| `src/OpenFOAM/db/IOstreams/token/token.C` | Defines `Compound<T>` registration |
| `src/OpenFOAM/fields/pointPatchFields/pointPatchField/pointPatchFieldNew.C` | pointPatchField registration |
| `src/OpenFOAM/global/globals.C` | DLL static init entry point (has trace output) |
| `src/OpenFOAM/global/debug/debug.C` | controlDict loading and debug switch init |
| `src/OpenFOAM/memoryPool/MemoryPool.C` | Memory pool getInstance() (has trace output) |
| `wmake/src/wmkdepend.cc` | Dependency generator (has .dep.part rename bug at line 1017) |
| `rebuild-openfoam-dll.sh` | Build script with all env vars |
| `stderr_test5.txt` | Latest test stderr output |

---

## Environment Requirements

- **MSYS2/MinGW64** shell for build commands
- **AdaptiveCpp (acpp)** at `/c/dev/llvm-acpp-msvc-21`
- **CUDA 12.5** (NOT 13.0+) at standard path
- **MSVC Build Tools 2022** for link.exe and libraries
- **Windows SDK 10.0.22621.0** for kernel32, advapi32, etc.

---

## Task List Status

| Task | Status | Description |
|------|--------|-------------|
| #12 | in_progress | Fix DLL static init failures |
| #13 | pending (blocked by #12) | Run blockMesh on pitzDaily tutorial |
| #14 | pending (blocked by #13) | Run simpleFoam on pitzDaily tutorial |

---

## What Success Looks Like

1. **Immediate goal**: `blockMesh.exe` runs without crash (even if it produces an error about missing case files)
2. **Next goal**: `blockMesh` successfully meshes the `tutorials/incompressible/simpleFoam/pitzDaily` case
3. **Final goal**: `simpleFoam` runs on the pitzDaily case with GPU acceleration

---

## Tips for the Next Agent

1. **Read Debugging-Conclusions.md first** -- it has all the patterns and anti-patterns for this codebase
2. **Don't use `FatalError` or `Info` in static initializers** -- they may not be initialized yet
3. **Always use PowerShell** to get actual exit codes, not MSYS2 bash
4. **The `.dep.part` rename workaround** is needed after every header change
5. **Build takes ~2 hours** -- plan your changes carefully before triggering a rebuild
6. **The duplicate entry warnings may be benign** -- focus on finding what actually causes the STATUS_DLL_INIT_FAILED after the warnings
7. **NEVER delete files/directories without explicit user confirmation** -- see CLAUDE.md safety rules
