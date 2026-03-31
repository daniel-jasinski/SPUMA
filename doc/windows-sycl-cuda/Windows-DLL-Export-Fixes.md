# Windows DLL Export Fixes for OpenFOAM/SPUMA

This document details all code changes required to make OpenFOAM compile and run correctly as Windows DLLs with cross-DLL static data access.

## The Problem

On Windows, DLLs are isolated compilation units. Static data members in C++ classes are NOT automatically shared across DLL boundaries. You must explicitly mark them with:
- `__declspec(dllexport)` when DEFINING (in the DLL that owns the data)
- `__declspec(dllimport)` when USING (in code that consumes the DLL)

Without this, you get:
- **0xC0000139 (STATUS_ENTRYPOINT_NOT_FOUND)**: Symbol not exported
- **0xC0000005 (STATUS_ACCESS_VIOLATION)**: Symbol exists but points to wrong memory

---

## File: src/OpenFOAM/include/stdFoam.H

Add these macro definitions (after the existing FOAM_DEPRECATED macros, around line 70):

```cpp
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Windows DLL Export Macros
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// FOAM_EXPORT_DATA: For extern data symbols (FatalError, Info, etc.)
// When building libOpenFOAM itself, define FOAM_BUILDING_LIB to use dllexport.
// Downstream DLLs use dllimport (the default).
#if defined(_WIN32)
# if defined(FOAM_BUILDING_LIB)
#  define FOAM_EXPORT_DATA __declspec(dllexport)
# else
#  define FOAM_EXPORT_DATA __declspec(dllimport)
# endif
#else
# define FOAM_EXPORT_DATA
#endif

// FOAM_TYPENAME_EXPORT: For TypeName/ClassName macros' static member declarations.
// On Windows, always use dllexport for ALL libraries (not just libOpenFOAM).
// This matches FOAM_DEFINE_STATIC_EXPORT so declaration and definition are consistent.
// Windows requires that if a definition has dllexport, the declaration must also have it.
#if defined(_WIN32)
# define FOAM_TYPENAME_EXPORT __declspec(dllexport)
#else
# define FOAM_TYPENAME_EXPORT
#endif

// FOAM_DEFINE_STATIC_EXPORT: For defining static members that need to be exported.
// Used in .C files when DEFINING static members.
#if defined(_WIN32)
# define FOAM_DEFINE_STATIC_EXPORT __declspec(dllexport)
#else
# define FOAM_DEFINE_STATIC_EXPORT
#endif
```

---

## File: src/OpenFOAM/db/error/error.H

Change extern declarations to use FOAM_EXPORT_DATA (around line 300+):

```cpp
// Before:
extern error FatalError;
extern IOerror FatalIOError;

// After:
FOAM_EXPORT_DATA extern error FatalError;
FOAM_EXPORT_DATA extern IOerror FatalIOError;
```

---

## File: src/OpenFOAM/db/error/messageStream.H

```cpp
// Around line 200+
// Before:
extern messageStream Info;
extern messageStream InfoErr;
extern messageStream Warning;
extern messageStream SeriousError;
extern int infoDetailLevel;

// After:
FOAM_EXPORT_DATA extern messageStream Info;
FOAM_EXPORT_DATA extern messageStream InfoErr;
FOAM_EXPORT_DATA extern messageStream Warning;
FOAM_EXPORT_DATA extern messageStream SeriousError;
FOAM_EXPORT_DATA extern int infoDetailLevel;
```

---

## File: src/OpenFOAM/db/Time/Time.H

```cpp
// Static member declaration
// Before:
static word controlDictName;

// After:
FOAM_EXPORT_DATA static word controlDictName;
```

---

## File: src/OpenFOAM/meshes/polyMesh/polyMesh.H

```cpp
// Static member declarations
// Before:
static word defaultRegion;
static word meshSubDir;

// After:
FOAM_EXPORT_DATA static word defaultRegion;
FOAM_EXPORT_DATA static word meshSubDir;
```

---

## File: src/OpenFOAM/db/IOstreams/IOstreams/IOstream.H

```cpp
// Static member
// Before:
static unsigned int precision_;

// After:
FOAM_EXPORT_DATA static unsigned int precision_;
```

---

## File: src/OpenFOAM/db/IOstreams/Pstreams/UPstream.H

```cpp
// Multiple static members - add FOAM_EXPORT_DATA to each:
// Before:
static bool parRun_;
static bool haveThreads_;
static int msgType_;
static label worldComm;
static label warnComm;
static DynamicList<int> myProcNo_;
static List<List<int>> procIDs_;
static List<label> parentComm_;

// After:
FOAM_EXPORT_DATA static bool parRun_;
FOAM_EXPORT_DATA static bool haveThreads_;
FOAM_EXPORT_DATA static int msgType_;
FOAM_EXPORT_DATA static label worldComm;
FOAM_EXPORT_DATA static label warnComm;
FOAM_EXPORT_DATA static DynamicList<int> myProcNo_;
FOAM_EXPORT_DATA static List<List<int>> procIDs_;
FOAM_EXPORT_DATA static List<label> parentComm_;
```

---

## File: src/OpenFOAM/primitives/strings/word/word.H

```cpp
// Add to static members
// Before:
static const char* const typeName;
static int debug;
static const word null;

// After:
FOAM_TYPENAME_EXPORT static const char* const typeName;
FOAM_TYPENAME_EXPORT static int debug;
FOAM_EXPORT_DATA static const word null;
```

---

## File: src/OpenFOAM/containers/HashTables/HashTable/HashTableCore.H

```cpp
// Change from:
static const label maxTableSize = 1L << 61;  // UB on Windows!

// To:
FOAM_EXPORT_DATA static const label maxTableSize;
```

And in HashTableCore.C:
```cpp
const Foam::label Foam::HashTableCore::maxTableSize = Foam::label(1) << 61;
```

---

## File: src/OpenFOAM/db/typeInfo/className.H

### ClassNameNoDebug macro (around line 45):
```cpp
// Before:
#define ClassNameNoDebug(TypeNameString)                                       \
    static const ::Foam::word typeName

// After:
#define ClassNameNoDebug(TypeNameString)                                       \
    FOAM_TYPENAME_EXPORT static const ::Foam::word typeName
```

### ClassName macro (around line 72):
```cpp
// Before:
#define ClassName(TypeNameString)                                              \
    ClassNameNoDebug(TypeNameString);                                          \
    static int debug

// After:
#define ClassName(TypeNameString)                                              \
    ClassNameNoDebug(TypeNameString);                                          \
    FOAM_TYPENAME_EXPORT static int debug
```

### defineTypeNameWithName macro (around line 98):
```cpp
// Before:
#define defineTypeNameWithName(Type, Name)                                     \
    const ::Foam::word Type::typeName(Name)

// After:
#define defineTypeNameWithName(Type, Name)                                     \
    FOAM_DEFINE_STATIC_EXPORT const ::Foam::word Type::typeName(Name)
```

---

## File: src/OpenFOAM/global/debug/defineDebugSwitch.H

### defineDebugSwitchWithName macro (around line 127):
```cpp
// Before:
#define defineDebugSwitchWithName(Type, Name, Value)                           \
    int Type::debug(::Foam::debug::debugSwitch(Name, Value))

// After:
#define defineDebugSwitchWithName(Type, Name, Value)                           \
    FOAM_DEFINE_STATIC_EXPORT int Type::debug(::Foam::debug::debugSwitch(Name, Value))
```

---

## File: src/OpenFOAM/global/argList/argList.H

Add `#include "stdFoam.H"` at the top, then annotate ALL static members:

```cpp
// Static Data Members
// Before:
static SLList<string> validArgs;
static HashSet<string> advancedOptions;
static HashTable<string> validOptions;
static HashTable<string> validParOptions;
static HashTable<std::pair<word,int>> validOptionsCompat;
static HashTable<std::pair<bool,int>> ignoreOptionsCompat;
static HashTable<string, label, Hash<label>> argUsage;
static HashTable<string> optionUsage;
static SLList<string> notes;
static std::string::size_type usageMin;
static std::string::size_type usageMax;
static word postProcessOptionName;

// After:
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

## Files with Direct `static int debug;` Declarations

These files declare `debug` directly without using TypeName/ClassName macros. They need `#include "stdFoam.H"` and modification:

| File | Change |
|------|--------|
| `src/OpenFOAM/db/functionObjects/functionObject/functionObject.H` | `FOAM_TYPENAME_EXPORT static int debug;` |
| `src/OpenFOAM/primitives/strings/string/string.H` | `FOAM_TYPENAME_EXPORT static int debug;` |
| `src/OpenFOAM/primitives/strings/fileName/fileName.H` | `FOAM_TYPENAME_EXPORT static int debug;` |
| `src/OpenFOAM/primitives/ranges/labelRange/labelRange.H` | `FOAM_TYPENAME_EXPORT static int debug;` |
| `src/OpenFOAM/matrices/schemes/schemesLookup.H` | `FOAM_TYPENAME_EXPORT static int debug;` |
| `src/OpenFOAM/matrices/solution/solution.H` | `FOAM_TYPENAME_EXPORT static int debug;` |
| `src/OpenFOAM/fields/GeometricFields/GeometricField/GeometricBoundaryField.H` | `FOAM_TYPENAME_EXPORT static int debug;` |

---

## Run-Time Selection Tables (~200+ files)

Files matching pattern `src/**/*New.C` use run-time selection macros that define static members. These macros are already fixed by the className.H and defineDebugSwitch.H changes above.

However, verify that each file compiles without "redeclaration cannot add dllexport" errors.

**Additionally**, see `Runtime-Selection-Table-Fix.md` for the function-based table access pattern required to prevent table duplication across DLLs.

---

## wmake Build System Changes

### File: wmake/makefiles/general

Add to linker flags:
```makefile
# Allow unresolved symbols for cross-DLL static data
LFLAGS += /FORCE:UNRESOLVED
EXE_LFLAGS += /FORCE:UNRESOLVED
```

### SYCL/CUDA Library Path

Create response file `sycl_libs.rsp` to handle spaces in CUDA path:
```
-L"C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5/lib/x64"
-lcudart
```

Reference in link command:
```makefile
$(LD) ... @sycl_libs.rsp
```

---

## Verification Checklist

After making changes:

1. [ ] Clean build directory: `rm -rf build/win64MsvcSyclDPInt32Opt/src/OpenFOAM`
2. [ ] Update lnInclude: `wmake/wmakeLnInclude -u src/OpenFOAM`
3. [ ] Rebuild libOpenFOAM: `cd src/OpenFOAM && wmake`
4. [ ] Test DLL loading with PowerShell LoadLibraryEx
5. [ ] Rebuild downstream libraries in dependency order
6. [ ] Rebuild simpleFoam
7. [ ] Test `simpleFoam -help`
8. [ ] Run actual CFD case

---

## Debugging Failed DLL Loading

### PowerShell Test Script
```powershell
$env:PATH = 'C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib;' +
            'C:\dev\llvm-acpp-msvc-21\bin;' +
            'C:\Program Files\NVIDIA GPU Computing Toolkit\CUDA\v12.5\bin;' +
            $env:PATH

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

$path = "C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib\libOpenFOAM.dll"
$handle = [NativeLib]::LoadLibraryEx($path, [IntPtr]::Zero, 0)
if ($handle -eq [IntPtr]::Zero) {
    $err = [NativeLib]::GetLastError()
    Write-Host "FAILED: Error $err (0x$($err.ToString('X')))"
} else {
    Write-Host "SUCCESS"
}
```

### Common Error Codes
| Error | Meaning | Solution |
|-------|---------|----------|
| 126 | ERROR_MOD_NOT_FOUND | Missing dependency DLL |
| 127 | ERROR_PROC_NOT_FOUND | Symbol not exported |
| 193 | ERROR_BAD_EXE_FORMAT | 32/64-bit mismatch |
