# Debugging Conclusions

This document records important conclusions, root causes, and diagnostic techniques discovered during debugging of the SPUMA Windows SYCL/CUDA build. Every agent performing debugging work on this project should append their findings here.

---

## Fix #1: controlDict writeUnits crash

**Symptom**: `blockMesh.exe` crashed immediately on startup with `STATUS_ACCESS_VIOLATION` during DLL static initialization.

**Root cause**: The `etc/controlDict` file was missing the `writeUnits` entry inside `DimensionSets/SICoeffs`. The `dimensionSets.C` static initializer tried to read this entry, got a null pointer, and crashed.

**Fix**: Added `writeUnits (kg m s K mol A Cd);` to the controlDict.

**Lesson**: Static initializers that read configuration files will crash silently if required entries are missing. Check controlDict completeness when porting to new platforms.

---

## Fix #2: MemoryPool getInstance() abort during static init

**Symptom**: Crash during DLL static initialization when `MemoryPool::getInstance()` was called before `MemoryPool::New()` created the singleton.

**Root cause**: Some `List`/`UList` code path with `usePool_=true` was executing during a static initializer in another compilation unit. On Windows, all static initializers in a DLL run under the loader lock, and the pool is not created until `main()` runs. The original code called `FatalErrorInFunction << abort(FatalError)`, which crashed.

**Fix**: Changed `getInstance()` to auto-create a `dummyMemoryPool(0)` when called before `New()`.

**Lesson**: On Windows, static initialization order within a DLL follows the link order of `.o` files. Code called during static init must not assume that singletons created by `main()` already exist.

---

## Fix #3: SYCL/CUDA init deadlock under Windows loader lock

**Symptom**: After fix #2, `blockMesh.exe` hung (timeout) instead of crashing. No output, no crash -- just a permanent hang.

**Root cause**: The `dummyMemoryPool` used `foamMemoryExecutor`, which is `syclMemoryExecutor` when `have_sycl=true`. Every memory operation called `getSyclQueue()`, which initializes CUDA. CUDA initialization requires thread creation and synchronization, which **deadlocks under the Windows loader lock** during DLL static initialization.

**Fix**: Changed `dummyMemoryPool.C` to use plain `std::malloc`/`std::free`/`std::memcpy`/`std::memset` instead of `foamMemoryExecutor` (SYCL). The `dummyMemoryPool` is a CPU-only fallback pool and does not need GPU memory.

**Diagnostic technique**: Used `reinterpret_cast<MemoryPool*>(0xDEAD0001)` as a dummy pointer. If something tried virtual dispatch on it, it would crash at the exact call site. This confirmed:
- The pool IS used during static init (crash occurred).
- The pool creation itself caused the hang (when a real `dummyMemoryPool` was created instead of the dummy pointer).

**Key insight -- Windows loader lock**: DLL static initializers (`DllMain` and all global constructors) run under the Windows loader lock. Operations that require thread creation, GPU initialization, or loading other DLLs will deadlock. This is a fundamental Windows constraint with no workaround other than deferring such operations to after DLL load completes.

---

## Fix #4: Pstream unresolved symbols causing DLL init failure

**Symptom**: After fix #3 (no more hang), `blockMesh.exe` exited immediately with `STATUS_DLL_INIT_FAILED` (exit code 127 in MSYS2 bash, 0xC0000142 as Windows NTSTATUS). DLL static initialization started (trace messages printed) but then the process was terminated by the loader.

**Root cause**: The `/FORCE:UNRESOLVED` linker flag created the DLL despite ~30 unresolved Pstream symbols. These symbols pointed to invalid addresses (typically address 0). When any static initializer (or code called by one) tried to call an unresolved Pstream function, it crashed. The Windows loader caught this exception during `DllMain` and returned `STATUS_DLL_INIT_FAILED`.

**Fix**: Created a proper MSVC-compatible static library using `llvm-ar rcs libPstream_static.lib *.o` (812 KB) from the individual Pstream `.o` files. Updated `Make/options` to link `libPstream_static.lib` on Windows. Note: `ld -r` combined `.o` files are incompatible with MSVC's `link.exe` (`LNK1220` error), and import libraries (`.lib` from DLL builds) don't contain actual code.

**Lesson**: `/FORCE:UNRESOLVED` creates a DLL that may load but will crash when unresolved symbols are called. Use `llvm-ar rcs` to create proper MSVC-compatible static libraries from `.o` files on Windows.

---

## Fix #5: Runtime selection table empty typeNames during static init

**Symptom**: Hundreds of "Duplicate entry  in runtime table" warnings with EMPTY type names, followed by `STATUS_DLL_INIT_FAILED`. The empty space between "entry" and "in" indicated that the type name was an empty string.

**Root cause**: The registration constructor default argument `const ::Foam::word& k = baseType##Type::typeName` references a static `const Foam::word` member that requires dynamic initialization (it's constructed from `typeName_()` which returns a `const char*`). During DLL static initialization, the registration constructors for many types ran BEFORE the `typeName` static member was dynamically initialized (zero-initialized `word` = empty string). This is the classic "static initialization order fiasco" on Windows DLLs.

**Fix**: Changed the default argument from `baseType##Type::typeName` to `baseType##Type::typeName_()` in all 6 registration constructor declarations across `runTimeSelectionTables.H` and `memberFunctionSelectionTables.H`. The `typeName_()` function returns a `const char*` string literal pointer (always available, no dynamic init needed), which is implicitly converted to a temporary `Foam::word` and bound by the `const &` parameter.

**Lesson**: On Windows DLLs, never use static data members as default arguments in constructors of other static objects. Use constexpr/inline functions that return literals instead. The C++ standard guarantees within-TU ordering of static init, but the MSVC CRT may not honor this across COMDAT-folded template instantiations.

---

## Fix #6: Duplicate syclGlobal.o in linker input

**Symptom**: `syclGlobal.o` appeared twice in `link_objects.rsp`.

**Root cause**: `Make/files` had `device/syclGlobal.cpp` both via `$(DEVICE_SOURCE)` (expanded by Make from `Make/deviceFiles`) AND as a hardcoded entry. The `sourceFiles` generation (cpp+sed) doesn't expand Make variables, so `$(DEVICE_SOURCE)` passed through to `sourceFiles` as a literal. Make then expanded it AND included the hardcoded entry, resulting in the file appearing twice.

**Fix**: Removed the hardcoded `device/syclGlobal.cpp` line from `Make/files`, since `$(DEVICE_SOURCE)` already adds it.

**Lesson**: Don't hardcode source files in `Make/files` that are already included via Make variable expansion (like `$(DEVICE_SOURCE)`). Check `link_objects.rsp` for duplicates with `sort | uniq -d`.

---

## Diagnostic Techniques

### PowerShell for Windows exit codes

MSYS2 bash truncates exit codes to 0-255. To get actual Windows NTSTATUS codes:

```powershell
$p = Start-Process -FilePath "blockMesh.exe" -Wait -PassThru
$p.ExitCode
# -1073741502 = 0xC0000142 = STATUS_DLL_INIT_FAILED
# -1073741819 = 0xC0000005 = STATUS_ACCESS_VIOLATION
```

### Common Windows NTSTATUS exit codes

| Exit code (decimal) | Hex | Name | Meaning |
|---|---|---|---|
| -1073741819 | 0xC0000005 | STATUS_ACCESS_VIOLATION | Null pointer dereference or bad memory access |
| -1073741502 | 0xC0000142 | STATUS_DLL_INIT_FAILED | DLL static init crashed or a required DLL failed to load |
| -1073741515 | 0xC0000135 | STATUS_DLL_NOT_FOUND | A dependent DLL was not found on PATH |
| 127 | N/A | MSYS2 generic | Usually maps to one of the above; use PowerShell for the real code |

### `__builtin_return_address(0)` for caller tracing

Works with clang/acpp to get the caller address without including `Windows.h` headers (which fail in CUDA device compilation):

```cpp
void* caller = __builtin_return_address(0);
fprintf(stderr, "Called from %p\n", caller);
```

### Dummy pointer technique for static init debugging

To determine whether a function is called during static init vs. normal runtime:

```cpp
static MemoryPool* instance_ = reinterpret_cast<MemoryPool*>(0xDEAD0001);
// If virtual dispatch occurs on this pointer during static init,
// it crashes at the exact call site, giving a precise stack trace.
```

### DLL dependency checking

```powershell
# List DLL imports
dumpbin /imports libOpenFOAM.dll | Select-String "\.dll"

# Check if specific symbols are exported
dumpbin /exports libOpenFOAM.dll | Select-String "symbolName"

# Check for unresolved symbols in object files
llvm-nm -u objectFile.o
```

### LoadLibraryEx isolation testing

Test each DLL independently to find which one fails during static init:

```powershell
$env:WM_PROJECT_DIR = "C:\OpenFOAM\SPUMA"
$env:PATH = "C:\OpenFOAM\SPUMA\platforms\win64MsvcSyclDPInt32Opt\lib;C:\dev\llvm-acpp-msvc-21\bin;$env:PATH"

Add-Type -TypeDefinition '
    using System;
    using System.Runtime.InteropServices;
    public class Kernel32 {
        [DllImport("kernel32", SetLastError=true)]
        public static extern IntPtr LoadLibraryEx(string path, IntPtr hFile, uint flags);
        [DllImport("kernel32")]
        public static extern int GetLastError();
    }'

$h = [Kernel32]::LoadLibraryEx("C:\...\libSomething.dll", [IntPtr]::Zero, 0)
if ($h -eq [IntPtr]::Zero) {
    Write-Host "FAILED: error $([Kernel32]::GetLastError())"
    # Error 1114 = ERROR_DLL_INIT_FAILED (static init crashed)
    # Error 126 = ERROR_MOD_NOT_FOUND (dependency missing)
} else {
    Write-Host "OK: handle $h"
}
```

This isolates which DLL has the init failure, since `blockMesh.exe` loads all DLLs and the error could come from any of them.

### Checking for stale `.dep.part` files

```bash
find /c/OpenFOAM/SPUMA/build -name "*.dep.part" | wc -l
# If > 0, dependencies are stale. Fix with:
find /c/OpenFOAM/SPUMA/build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;
```

---

## Build System Lessons

1. **wmake variable passing**: GNU Make does NOT inherit shell environment variables. `wmake` passes them as explicit `VAR="$VAR"` pairs on the command line at three locations (sourceFiles generation, dependency update, main compile/link).

2. **sourceFiles generation**: The `sourceFiles` file is generated by `wmake/makefiles/files` using cpp preprocessing + sed extraction. It does NOT expand Make variables like `$(DEVICE_SOURCE)`. Use `#ifdef` guards in C++ source code instead.

3. **Windows system libraries**: Use `-Wl,/DEFAULTLIB:libname` instead of bare `libname.lib` in Make/options. Clang++ (acpp backend) tries to open bare `.lib` files as input files.

4. **WIN_* link variables**: wmake computes SDK/MSVC/CUDA paths with `cygpath -w -s` (8.3 short names) for MSVC's `LIB` environment variable.

5. **Static initialization order on Windows**: Within a DLL, follows link order of `.o` files. Between DLLs, determined by import dependencies. Code in DLL static initializers must not assume anything about runtime state.

6. **`/FORCE:UNRESOLVED` linker flag**: Creates a DLL despite unresolved symbols. The DLL may load but will crash when unresolved symbols are called. Only useful as a temporary build diagnostic.

7. **DEF file generation**: Already integrated via `wmake/scripts/generate-msvc-def`, which extracts symbols from object files using `llvm-nm`.

---

## Fix #7: Downstream DLLs built before Fix #5 still have static init fiasco

**Symptom**: After Fix #5 and Fix #6, `libOpenFOAM.dll` loads successfully (confirmed via PowerShell `LoadLibraryEx`), but `blockMesh.exe` still exits with `STATUS_DLL_INIT_FAILED` (exit code 127 / 0xC0000142).

**Root cause**: Downstream DLLs (libfiniteVolume, libmeshTools, libdynamicMesh, libblockMesh) were compiled **before** Fix #5 changed the registration constructor default argument from `typeName` to `typeName_()` in the headers. These stale DLLs still contained the old header code, so their registration constructors still suffered from the static init order fiasco (referencing uninitialized `typeName` members).

**Diagnostic technique - LoadLibraryEx isolation**: Used PowerShell to test each DLL independently:
```powershell
$env:WM_PROJECT_DIR = "C:\OpenFOAM\SPUMA"
$env:PATH = "...\lib;...\bin;$env:PATH"
Add-Type -TypeDefinition '
    using System.Runtime.InteropServices;
    public class Kernel32 {
        [DllImport("kernel32", SetLastError=true)]
        public static extern IntPtr LoadLibraryEx(string path, IntPtr hFile, uint flags);
    }'
$h = [Kernel32]::LoadLibraryEx("C:\...\libOpenFOAM.dll", [IntPtr]::Zero, 0)
# Returns non-zero handle = SUCCESS
$h = [Kernel32]::LoadLibraryEx("C:\...\libmeshTools.dll", [IntPtr]::Zero, 0)
# Returns 0, GetLastError = 1114 = ERROR_DLL_INIT_FAILED
```

Results:
- libOpenFOAM.dll: **OK** (loads fine with handle returned)
- libfileFormats.dll: **OK**
- libsurfMesh.dll: **OK**
- libmeshTools.dll: **FAILED** (error 1114)
- libfiniteVolume.dll: **FAILED** (error 1114)
- libdynamicMesh.dll: **FAILED** (error 1114)
- libblockMesh.dll: **FAILED** (error 1114)

**Fix**: Rebuild all downstream DLLs with current headers containing the `typeName_()` fix.

**Lesson**: When header-level fixes change registration macros, ALL downstream DLLs that use those macros must be rebuilt. DLL timestamps can be compared against the header fix date to identify stale DLLs. Use LoadLibraryEx to isolate which specific DLL is failing.

---

## Observation: Compound<T> duplicate entries are benign (Fix #5 regression)

**Symptom**: 12x "Duplicate entry Compound<T> in runtime table compound" warnings during DLL static init.

**Root cause**: `token.H` uses `TypeNameNoDebug("Compound<T>")` which makes `typeName_()` return the literal `"Compound<T>"` for ALL template instantiations (e.g., `Compound<scalarList>`, `Compound<vectorList>`, etc.). Before Fix #5, the `defineCompoundTypeName` macro specialized `typeName` to per-type names (e.g., `"scalarList"`), and the registration constructor used `typeName` as the key. Fix #5 changed the default arg to `typeName_()`, causing all 12 Compound instantiations to register with the same key `"Compound<T>"`.

**Impact**: The duplicates are **benign** -- the insertion fails silently and falls through. The Compound types are looked up by their IOstream token type, not by runtime selection table name. The warning messages are cosmetic.

**Lesson**: `TypeNameNoDebug` with a fixed string shared across template instantiations is incompatible with per-specialization `typeName` members as registration keys. This is a known limitation of Fix #5.

---

## Observation: Empty pointPatchField entries from coupledPointPatchField

**Symptom**: "Duplicate entry  in runtime table pointPatchField" with empty type name.

**Root cause**: `coupledPointPatchField` uses `TypeName(coupledPointPatch::typeName_())` -- a function-based TypeName. The `coupledPointPatch::typeName_()` function returns `"coupled"`, but since `TypeName` is a macro that expands at class definition time, and the function is not constexpr, the name registration happens at static init time when the function result may not yet be available in the same way.

**Status**: Under investigation. The empty name suggests another variant of the static init order issue.

---

## Observation: wmkdepend `.dep.part` rename failure on Windows

**Symptom**: After header changes, `wmake` doesn't recompile files that include the changed headers. Object files remain stale.

**Root cause**: `wmkdepend` writes dependency files as `.dep.part` then calls `rename()` to atomically move them to `.dep`. On Windows/MSYS2, the `rename()` call fails silently (possibly due to file locking or permission issues), leaving `.dep.part` files that `make` never reads.

**Workaround**: After header changes, manually rename all `.dep.part` files:
```bash
find /c/OpenFOAM/SPUMA/build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;
```

**Lesson**: Always check for `.dep.part` files before rebuilding after header changes. If they exist, dependencies are stale and files won't be recompiled.

---

## Fix #8: Cross-DLL data symbol access via JMP thunk (usageTablePtr_)

**Symptom**: `libdynamicMesh.dll` crashes during DLL static initialization with `ACCESS_VIOLATION` (0xc0000005) at a consistent offset. All dependency DLLs load successfully. The link was 100% clean (zero LNK4088 warnings for non-/FORCE issues).

**Root cause**: `topoSetSource::usageTablePtr_` is a `static HashTable<string>*` defined in `libmeshTools.dll` (in `topoSetSource.C`) and accessed from `libdynamicMesh.dll` (via `addToUsageTable` static constructors in `badQualityToCell`, `badQualityToFace`, etc.).

On MSVC, when a DLL exports a data symbol via DEF file without the `DATA` keyword, the import library creates a **JMP thunk** (function-style import) at the symbol address in the importing DLL. Code in `libdynamicMesh.dll` that reads `usageTablePtr_` actually reads from the thunk address in `.text`, getting JMP instruction bytes (`ff 25 xx xx xx xx`) interpreted as a pointer value. This garbage pointer is then dereferenced in `HashTable::setEntry()`, causing the ACCESS_VIOLATION.

**Diagnosis technique**: Used `llvm-objdump -d` to disassemble at the crash RVA (from Windows Event Log fault offset). Found the crash in `HashTable<string, word>::setEntry()` at `cmpl $0x0, 0x4(%rcx)`. Traced back through the caller to find the global variable at RVA `0x48a8b0`. Checked section boundaries with `llvm-readobj --sections` - the RVA fell in `.text` (code), not `.data`. Disassembled the address and found a JMP thunk confirming the import mechanism failure.

**Investigation of DATA keyword**: Tried adding `DATA` keyword to DEF file for D/B/C/R type symbols. This caused worse behavior: the import library only creates `__imp_` prefixed entries for DATA exports, and without `__declspec(dllimport)` in the source code, the linker can't find the unprefixed symbol names at all, producing many LNK2019 (unresolved external) errors. The DATA keyword approach was reverted.

**Fix**: Converted `usageTablePtr_` from a raw static class member to a **function-local static accessor** (same pattern used for runtime selection tables):
```cpp
// In topoSetSource.H:
static HashTable<string>*& usageTablePtr_();  // was: static HashTable<string>* usageTablePtr_;

// In topoSetSource.C:
Foam::HashTable<Foam::string>*& Foam::topoSetSource::usageTablePtr_()
{
    static HashTable<string>* ptr = nullptr;
    return ptr;
}
```
All accesses changed from `usageTablePtr_` to `usageTablePtr_()`. The function call goes through the normal import mechanism (function thunks work correctly), and the function-local static ensures all DLLs share the same pointer instance.

**Lesson**: On Windows MSVC DLLs without `__declspec(dllimport)`, cross-DLL **data** access is fundamentally broken:
- Without `DATA` keyword in DEF: the import lib creates a JMP thunk; code reads instruction bytes as data (garbage)
- With `DATA` keyword in DEF: the import lib only provides `__imp_` entries; code without dllimport can't find the symbol (LNK2019)
- The only reliable approach is to **access cross-DLL data through functions** (function-local statics, accessor methods). Function imports work correctly because the JMP thunk is called (not read as data).
- This is the same pattern already used for runtime selection tables (`TablePtr_()` returning `static ptr`).

---

## Fix #9: DEF file R-type symbol filtering for MSVC 65535 export limit

**Symptom**: `libfiniteVolume.dll` link failed with `LNK1189: library limit of 65535 objects exceeded` (90,469 symbols in DEF file).

**Root cause**: Adding R-type (read-only data) symbols to the DEF generator (Fix #10 in MEMORY.md) included many compiler-internal R-type symbols that don't need cross-DLL export: RTTI structures (??_R, 18K symbols), vftables (??_7, 4.5K), string literals (??_C, 4K), anonymous symbols (??@, 4K), etc.

**Fix**: Added comprehensive filtering to `wmake/scripts/generate-msvc-def`:
- `??_R` - ALL RTTI structures (not just R0; OpenFOAM uses its own runtime selection)
- `??_7` - vftables (accessed via vtable pointer in object, not by symbol name)
- `??_8` - vbtables (virtual base tables)
- `??_C` - string literals (compiler-generated)
- `??@` - anonymous/hash-named symbols
- `__real@` - floating-point constants
- `__xmm@` - SSE/SIMD constants
- `_CT`, `_CTA`, `_TI` - exception handling structures

Result: libOpenFOAM DEF went from 44,831 to 25,365 symbols; libfiniteVolume from 90,469 to 52,699 (under 65K limit). Verified that needed symbols (pTraits::zero, VectorSpace::typeName) survived the filters.

**Lesson**: When exporting R-type symbols via DEF files, filter aggressively. Only user-defined const data needs export; compiler-generated R-type symbols (RTTI, vftables, string literals, float/SIMD constants, exception handling) are TU-internal.

---

## Diagnostic Technique: Symbolizing DLL crash addresses

To identify which function is crashing from a Windows Event Log fault offset:

```bash
# 1. Get the crash RVA from Windows Event Log
#    (Event Viewer → Windows Logs → Application → "Faulting module" entry)
#    Fault offset: 0x00000000003f450a

# 2. Get the DLL's image base
llvm-readobj --file-headers libDLL.dll | grep ImageBase
# ImageBase: 0x180000000

# 3. Compute the virtual address (VA = ImageBase + RVA)
# VA = 0x180000000 + 0x3f450a = 0x1803f450a

# 4. Disassemble at that address
llvm-objdump -d --start-address=0x1803f450a --stop-address=0x1803f4560 libDLL.dll

# 5. Demangle the function name
echo '??$setEntry@...' | llvm-undname

# 6. Check section boundaries to verify address is in correct section
llvm-readobj --sections libDLL.dll | grep -E "Name:|VirtualAddress|VirtualSize"
```

If the data variable address falls in `.text` instead of `.data`, this indicates the linker created a JMP thunk (function import) for a data symbol - see Fix #8.

---

## Fix #10: IOobjectTemplates.C Type::typeName cross-DLL data access crash

**Symptom**: blockMesh.exe crashed in `meshDictIO.typeHeaderOk<IOdictionary>(true)` during `findBlockMeshDict.H`. Segfault with no useful error message.

**Root cause**: `typeHeaderOk<Type>` is a template in `IOobjectTemplates.C`, instantiated in the consumer (blockMesh.exe or downstream DLLs). It accesses `Type::typeName`, which is a `const word` static data member. On Windows MSVC, `FOAM_TYPENAME_EXPORT` is ALWAYS `__declspec(dllexport)` (in `stdFoam.H` line 95), even in consumer code that should use `dllimport`. This causes the compiler to create a local uninitialized copy instead of importing from the DLL where the data actually lives. Reading this uninitialized word object causes a crash.

**Fix**: Changed all `Type::typeName` references in `IOobjectTemplates.C` to use `Type::typeName_()` (function returning `const char*`, wrapped in `Foam::word()`). Functions go through JMP thunks correctly; only data access is broken. Changed in three template methods: `typeHeaderOk`, `typeFilePath`, and `warnNoRereading`.

```cpp
// BEFORE (crashes on Windows):
Type::typeName,
// AFTER (works):
Foam::word(Type::typeName_()),
```

**Lesson**: Never access `Type::typeName` (static data member) from template code that may be instantiated in consumer DLLs. Always use `Type::typeName_()` (function call) instead. This is a generalization of Fix #5 (which was about default arguments, this is about explicit access).

---

## Fix #11: dictionary::writeOptionalEntries cross-DLL data access

**Symptom**: After Fix #10, blockMesh progressed further but hit: `FOAM FATAL IO ERROR: No optional entry: verbose Default: 1`. The `writeOptionalEntries` variable should have been 0 (set via controlDict), but was reading as a value > 1.

**Root cause**: `dictionary::writeOptionalEntries` is a `static int` without any export annotation. When accessed from template code (`dictionaryTemplates.C`) instantiated in consumer DLLs, the access goes through a JMP thunk created by the DEF file. Reading JMP instruction bytes (`ff 25 xx xx xx xx ...`) as an int yields a large value > 1, triggering the `FatalIOError` branch.

**Fix**: Added `FOAM_EXPORT_DATA` annotation to `writeOptionalEntries`, `null`, and `reportingOutput` in `dictionary.H`. `FOAM_EXPORT_DATA` = `__declspec(dllexport)` when `FOAM_BUILDING_LIB` (only libOpenFOAM), `__declspec(dllimport)` otherwise. Consumer DLLs compiled with this header will generate correct `__imp_` prefixed imports.

```cpp
FOAM_EXPORT_DATA static int writeOptionalEntries;
FOAM_EXPORT_DATA static const dictionary null;
FOAM_EXPORT_DATA static refPtr<OSstream> reportingOutput;
```

**Lesson**: Static data members accessed from template code across DLL boundaries MUST have proper dllexport/dllimport annotations. The DEF file alone creates function-style thunks that break data access.

---

## Fix #12: dictionary template accessor functions for cross-DLL safety

**Symptom**: Even with `FOAM_EXPORT_DATA` on `writeOptionalEntries` (Fix #11), every downstream DLL that instantiates dictionary templates (getOrDefault, getOrAdd, etc.) must be rebuilt to pick up the dllimport annotation. Additionally, inline accessor functions (`reportOptional()`) in headers still compile the data access into consumer code.

**Root cause**: `dictionaryTemplates.C` directly accesses `writeOptionalEntries` and `reportingOutput` (static data members). These templates are instantiated in consumer DLLs. Even with the `FOAM_EXPORT_DATA` annotation, inline accessors like `reportOptional()` (defined in `dictionaryI.H`) get inlined into consumer code, putting the data access back in the consumer DLL.

**Fix**:
1. Made `reportOptional()` non-inline (moved definition from `dictionaryI.H` to `dictionary.C`)
2. Added new `reportingOutputStream()` non-inline accessor (defined in `dictionary.C`)
3. Changed `dictionaryTemplates.C` to use `reportOptional()` instead of `writeOptionalEntries` and `reportingOutputStream()` instead of `InfoErr.stream(reportingOutput.get())`

Now all data access is compiled into libOpenFOAM.dll (where the data lives), and template code only makes function calls through import thunks (which work correctly).

**Lesson**: For data accessed from template code across DLL boundaries, the most robust approach is to provide non-inline accessor functions compiled in the defining DLL. This avoids relying on correct dllimport annotations in all consumers and works even if consumer DLLs are not rebuilt with updated headers.

---

## Fix #13: nullObjectPtr cross-DLL data access crash in iterator_end()

**Symptom**: After Fixes #10-#12, blockMesh progressed through dictionary reading and findBlockMeshDict but crashed during `searchableSurfaces` constructor initialization (geometry_ member). Crash at `movq 0x8(%rcx), %rax` where `rcx` was loaded from a JMP thunk address in `.text` section of libmeshTools.dll. Event Log showed fault in libmeshTools.dll at offset 0x1a298f.

**Root cause**: `nullObjectPtr` is an `extern const NullObject*` global defined in libOpenFOAM.dll (in `nullObject.C`). It's used extensively in inline template functions throughout the codebase:
- `DLListBase::iterator_end<>()` in `DLListBaseI.H` - returns end sentinel for linked lists
- `SLListBase::iterator_end<>()` in `SLListBaseI.H` - same for singly-linked lists
- `NullObjectPtr<T>()`, `NullObjectRef<T>()`, `isNull()`, `isNotNull()` in `nullObject.H`

When iterating an empty dictionary with a range-based for loop, the compiler generates a call to `end()` which calls `iterator_end<>()`. This inline function accesses `nullObjectPtr` from the consumer DLL (libmeshTools.dll). Without proper dllimport annotation, the linker creates a JMP thunk for the data symbol. The code reads JMP instruction bytes (`ff 25 ...`) as a pointer value, then dereferences it → crash.

The crash specifically happens on the `je` path (when dictionary is empty, size == 0), because only then does the code need the end-iterator sentinel from `nullObjectPtr`.

**Fix**: Added `FOAM_EXPORT_DATA` to `nullObjectPtr` declaration in `nullObject.H`. Also added `#include "stdFoam.H"` for the macro. With this annotation, libOpenFOAM sees `__declspec(dllexport)` (via `FOAM_BUILDING_LIB`), and consumer DLLs see `__declspec(dllimport)`, causing the compiler to generate correct `__imp_nullObjectPtr` IAT access.

```cpp
// BEFORE:
extern const NullObject* nullObjectPtr;
// AFTER:
FOAM_EXPORT_DATA extern const NullObject* nullObjectPtr;
```

Requires rebuilding libOpenFOAM.dll + ALL downstream DLLs.

**Lesson**: Any `extern` global data variable used in inline/template code that crosses DLL boundaries MUST have `FOAM_EXPORT_DATA` annotation. The `nullObjectPtr` pattern is especially dangerous because it's used in fundamental iteration primitives (linked list `end()` iterators) that are called from virtually everywhere. Other extern data accessed from inline code that may need the same fix: `word::debug`, `token::SPACE`, etc.

---

## Fix #14: blockMeshTopology.C cross-DLL typeName access

**Symptom**: blockMesh exit code 127 (SIGSEGV mapped by MSYS2), crash in `createTopology()` at `word defaultPatchType = emptyPolyPatch::typeName;`. Trace showed "TRACE:topo 2 - before emptyPolyPatch::typeName" was the last output.

**Root cause**: `emptyPolyPatch::typeName` is a `const word` static member defined in libOpenFOAM.dll. When accessed from libblockMesh.dll, the DEF-exported symbol resolves to a JMP thunk. Reading the thunk bytes as `word` (std::string) data causes a crash - same pattern as Fixes #8, #10, #13.

**Fix**: Changed `emptyPolyPatch::typeName` to `emptyPolyPatch::typeName_()` in blockMeshTopology.C. Also changed `cyclicPolyPatch::typeName` to `cyclicPolyPatch::typeName_()`. The `typeName_()` function returns a `const char*` string literal. Function calls through JMP thunks work correctly; only data reads through thunks are broken.

**Lesson**: **Every cross-DLL `::typeName` reference must use `::typeName_()` instead.** This is not just for registration macros (Fix #5) but also for runtime code that compares or copies type names. The `typeName_()` pattern is the universal safe replacement for `typeName` in cross-DLL code on Windows MSVC.

---

## Fix #15: FlexLexer.h not found during CUDA device pass

**Symptom**: Rebuilding libfileFormats.dll failed with `fatal error: 'FlexLexer.h' file not found` when compiling `STLAsciiParseFlex.L.C` for `sm_86` (CUDA device pass).

**Root cause**: FlexLexer.h is in `/c/msys64/usr/include/`, which is in the host compiler's default system include path but NOT in the CUDA device compiler's include path. The acpp dual-pass compilation (host + CUDA) compiles all files for both targets.

**Fix**: Copied `FlexLexer.h` from `/c/msys64/usr/include/` to `src/fileFormats/lnInclude/`. The `-IlnInclude` flag in wmake's compile command finds it there for both host and device passes. Note: Adding `-isystem /c/msys64/usr/include` to Make/options does NOT work because the MSYS2 C library headers (stdlib.h, etc.) conflict with MSVC headers.

**Lesson**: For dual-pass SYCL/CUDA compilation, system headers only available in MSYS2 paths must be copied to a project-local include directory. Never add MSYS2's system include path as `-isystem` because it conflicts with MSVC standard library headers.

---

## Fix #16: GeometricBoundaryField.C cross-DLL typeName in template code (PENDING REBUILD)

**Symptom**: simpleFoam crashes (SIGSEGV, exit 139) during `volScalarField p(IOobject, mesh)` construction. Debug traces show "TRACE:sf 2 - before volScalarField p ctor" but not "TRACE:sf 3 - after volScalarField p". stdout shows "Reading field p" as last output.

**Root cause**: `GeometricBoundaryField.C` is template code (included via `#ifdef NoRepository`). It's instantiated in `libfiniteVolume.dll` via `volFields.C` which explicitly instantiates `GeometricField<scalar, fvPatchField, volMesh>`. The template code references:
- `emptyPolyPatch::typeName` (line 279, 286) — comparing/passing patch type
- `cyclicPolyPatch::typeName` (line 319) — checking for split cyclics

These are `const word` static members defined in `libOpenFOAM.dll`. When accessed from template code compiled into `libfiniteVolume.dll`, the DEF-exported symbols resolve to JMP thunks. Reading thunk bytes as `word` data → crash. Same pattern as Fixes #8, #10, #13, #14.

**Fix**: Changed to `emptyPolyPatch::typeName_()` and `cyclicPolyPatch::typeName_()` in `GeometricBoundaryField.C`. Copied updated file to `src/OpenFOAM/lnInclude/`. **NOT YET COMPILED**: `volFields.o` (Feb 12 11:52) predates the fix. Must delete `volFields.o` and rebuild `libfiniteVolume.dll`.

**Lesson**: Template code in `src/OpenFOAM/` that's instantiated in downstream DLLs via `#ifdef NoRepository` is effectively cross-DLL code. All `::typeName` references in such templates must use `::typeName_()`. Key template files to audit: `GeometricField.C`, `GeometricBoundaryField.C`, `DimensionedField.C`, `Field.C`.

---

## Adding New Entries

When debugging issues in this project, append your findings to this document following this template:

```markdown
## Fix #N: Short description

**Symptom**: What you observed (crash, hang, wrong output, etc.)

**Root cause**: The actual underlying problem.

**Fix**: What was changed and why.

**Lesson**: General principle or insight for future debugging.
```

Include diagnostic techniques in the "Diagnostic Techniques" section if they are reusable.
