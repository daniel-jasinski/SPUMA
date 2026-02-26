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

## Fix #17: Comprehensive cross-DLL `::typeName` sweep

**Symptom**: simpleFoam still crashes with SIGSEGV during volScalarField constructor ("Reading field p") after Fix #16. Crash is in MSVC memcpy (`vmovdqu (%rdx),%ymm0`) in libOpenFOAM.dll at offset 0xc3cc1d, suggesting a bad pointer from JMP thunk.

**Root cause**: Multiple categories of cross-DLL `::typeName` data accesses existed throughout the codebase:

1. **polyPatch typeName in downstream .C files** (~20 instances): `emptyPolyPatch::typeName`, `processorPolyPatch::typeName`, `wallPolyPatch::typeName`, etc. in files like `fvMeshTools.C`, `voxelMeshSearch.C`, `snappyLayerDriver.C`, `STARCDMeshReader.C`, `blockMeshCreate.C`, `PDRblock.C`, `thermalBaffleFvPatchScalarField.C`, etc.

2. **pTraits<T>::typeName in template code** (~35 instances): `UListIO.C`, `FixedListIO.C`, `exprResultI.H`, `FieldField.C`, `FieldM.H`, `dynamicCode.H`, `functionObjectPropertiesTemplates.C`, etc. These template files get compiled into downstream DLLs, making `pTraits<T>::typeName` a cross-DLL data access.

3. **cyclicAMIPolyPatch::typeName in template code**: `particleTemplates.C` (lagrangian).

**Fix**: Three-pronged approach:

1. Changed all `::typeName` to `::typeName_()` in downstream .C files (13 source files, ~25 instances).
2. Added `static const char* typeName_()` inline function to ALL pTraits primitive specializations: `Scalar.H`, `bool.H`, `char.H`, `int32.H`, `int64.H`, `uint32.H`, `uint64.H`, `int8.H`, `uint8.H`, `complex.H`. For int/uint types, used `#if WM_LABEL_SIZE` conditionals to return the correct name ("label"/"int32" etc.).
3. Changed `pTraits<Type>::typeName` to `pTraits<Type>::typeName_()` in all cross-DLL template files (11 files, ~35 instances).

**Lesson**: The cross-DLL JMP thunk issue affects ALL static data members accessed across DLL boundaries, not just OpenFOAM class `typeName`. The `pTraits<T>::typeName` pattern is particularly insidious because pTraits specializations for primitive types (scalar, label, bool, etc.) don't use the `ClassName` macro and therefore lack `typeName_()`. Any template code accessing `pTraits<T>::typeName` that gets compiled in a downstream DLL is a ticking time bomb. **Always audit pTraits accesses when adding new template code.**

---

## Fix #18: `dimless` cross-DLL data access in GeometricField/DimensionedField constructors

**Symptom**: simpleFoam crashes during `volScalarField p(IOobject(...), mesh)` construction with segfault (exit code 139). Trace shows "TRACE:sf 2 - before volScalarField p ctor" as last output.

**Root cause**: `GeometricField.C` line 514 uses `Internal(io, mesh, dimless, false)` in the read constructor. `dimless` is a global `extern const dimensionSet` defined in `libOpenFOAM.dll`. Since `GeometricField.C` is template code compiled into downstream DLLs (via `#ifdef NoRepository`), this is a cross-DLL data access through a JMP thunk → crash.

Same issue at:
- `GeometricField.C:554` (dictionary constructor)
- `DimensionedFieldIO.C:122,141` (initializer `dimensions_(dimless)`)
- `UniformDimensionedField.C:103`

**Fix**: Two-pronged approach:
1. **Template code**: Replaced `dimless` with `dimensionSet()` (default constructor produces identical zero-dimension set). This eliminates cross-DLL data access entirely.
2. **Comprehensive**: Added `FOAM_EXPORT_DATA` annotation to ALL 23 dimension globals in `dimensionSets.H` (`dimless`, `dimMass`, `dimLength`, `dimTime`, `dimVelocity`, etc.). This provides proper `__declspec(dllimport)` annotation so downstream code uses `__imp_` references via the import table instead of JMP thunks.

Also replaced `dimless` with `dimensionSet()` in:
- `DimensionedScalarField.C` (8 instances)
- `GeometricScalarField.C` (8 instances)
- `geometricOneField.H`, `geometricZeroField.H` (inline `return dimless` → function-local static)

**Lesson**: ALL `extern` global data from `libOpenFOAM.dll` accessed in template code is a potential JMP thunk crash. The `dimensionSet` globals (`dimless`, `dimTime`, etc.) are particularly dangerous because they're commonly used in field constructors. The proper fix is `FOAM_EXPORT_DATA` on the declarations, but replacing with default-constructed equivalents in template code provides immediate safety without rebuilding libOpenFOAM.

---

## Fix #19: Comprehensive `typeName` sweep in template code

**Symptom**: Preventive fix - multiple template .C files under `src/OpenFOAM/` still contained bare `typeName` accesses that would crash if executed in downstream DLLs.

**Root cause**: Many template files use `typeName` (static `word` data member) for header checking, error messages, and I/O. When template code is compiled into downstream DLLs, `typeName` is a cross-DLL data access via JMP thunk.

**Fix**: Changed `typeName` to `typeName_()` in:
- `DimensionedFieldIO.C:82` (readContents call)
- `UniformDimensionedField.C:49,65,81` (readHeaderOk calls)
- `GlobalIOField.C` (5 readHeaderOk calls)
- `GlobalIOList.C` (4 readHeaderOk calls)
- `CompactIOField.C` (7 instances - read path, error messages, and const_cast write path)
- `CompactIOList.C` (7 instances - same pattern)
- `objectRegistryTemplates.C:619,632` (error messages)
- `indexedOctree.C:2302`, `dynamicIndexedOctree.C:2740` (debug output)
- `cyclicPointPatchField.C:84`, `emptyPointPatchField.C:79`, `wedgePointPatchField.C:81`, `symmetryPointPatchField.C:79`, `symmetryPlanePointPatchField.C:82` (error messages)

Also added `typeName_()` to `VectorSpace.H` and generic `pTraits.H` template to support `pTraits<Vector<double>>::typeName_()` and similar.

**Lesson**: ANY `typeName` reference in a file included via `#ifdef NoRepository` is a cross-DLL ticking time bomb. Do a codebase-wide grep for `\btypeName\b` in all template .C files periodically to catch new instances.

---

## Fix #20: `DebugInFunction` accessing `debug` via JMP thunk

**Symptom**: Segfault during `DebugInFunction` in GeometricField read constructor. `debug` value read as garbage (e.g., 1154360831) instead of 0, causing `if(debug)` to be true and crashing in the stream output code.

**Root cause**: Template code (GeometricField constructor) compiled in simpleFoam.o accesses the `debug` static member of template classes (e.g., `volScalarField::debug` defined in libfiniteVolume.dll) via JMP thunk. The thunk's instruction bytes are read as a garbage non-zero int.

**Fix**: Disabled all Debug* macros on Windows in `messageStream.H` using `#ifdef _WIN32` guards: `DebugInfo` → `if (false) Info`, `DebugInFunction` → `if (false) InfoInFunction`, `DebugPout` → `if (false) Pout`, `DebugPoutInFunction` → `if (false) PoutInFunction`, `DebugVar(var)` → `((void)0)`.

**Lesson**: ANY static data member of a template class instantiated in a different DLL is accessed via JMP thunk without `__declspec(dllimport)`. Debug macros that check `debug` are particularly dangerous because the garbage non-zero value triggers code paths that crash.

---

## Fix #21: `readFields()` type name mismatch after `typeName` → `typeName_()` change

**Symptom**: After Fix #20, simpleFoam exits with error "unexpected class name volScalarField expected GeometricField".

**Root cause**: `readFields()` was using `typeName_()` (which returns the base template name "GeometricField") instead of `typeName` (which returns the specialized name "volScalarField"). Can't use `typeName` (cross-DLL crash) and can't use `typeName_()` (wrong value).

**Fix**: Modified `readFields()` in GeometricField.C to construct `localIOdictionary` with `word()` (empty) to bypass the header class name check entirely. The `readStream()` function skips type validation when `expectName` is empty.

**Lesson**: When replacing `typeName` with `typeName_()` in template code, check whether the VALUE matters. For `readContents()`, the type name is used for validation, so an empty word bypass is needed. Note: `word::null` is also an `extern` variable → cross-DLL access. Use `word()` (default constructor) instead.

---

## Fix #22: `fieldTypes::calculatedType` and other `extern const word` without `FOAM_EXPORT_DATA`

**Symptom**: Segfault during `surfaceScalarField` construction. Crash at AVX2 memcpy instruction in libOpenFOAM.dll. `DimensionedField` construction works but full `GeometricField` (with boundary field) crashes.

**Root cause**: `fieldTypes::calculatedType` is an `extern const word` in libOpenFOAM.dll WITHOUT `FOAM_EXPORT_DATA`. The inline function `fvsPatchField<Type>::calculatedType()` returns a reference to it. This function is used as the DEFAULT PARAMETER for every GeometricField constructor: `const word& patchFieldType = PatchField<Type>::calculatedType()`. Template code in downstream DLLs/EXEs evaluates this default → reads JMP thunk bytes as a `word` object → crash during boundary field construction.

**Fix**: Added `FOAM_EXPORT_DATA` to all `extern` declarations in `fieldTypes.H`: `calculatedType`, `emptyType`, `extrapolatedCalculatedType`, `processorType`, `zeroGradientType`, and the `basic` wordList. Also added `#include "stdFoam.H"` for the FOAM_EXPORT_DATA macro definition. Required recompilation of ~123 .o files in libfiniteVolume.dll.

**Lesson**: ANY `extern` data in libOpenFOAM.dll that is accessed from downstream code MUST have `FOAM_EXPORT_DATA`. Inline functions that return references to extern data are particularly insidious because they look like function calls but actually access data cross-DLL. Check ALL headers in `src/OpenFOAM` for `extern const` without `FOAM_EXPORT_DATA`.

**Diagnostic technique**: Use `llvm-nm <file>.o | grep "symbolName"` and check for `__imp_` prefix. If present, `dllimport` is working. If absent, the access goes through JMP thunk. Use `for f in $(find build -name "*.o"); do if llvm-nm "$f" | grep -q "bareSymbol" && ! llvm-nm "$f" | grep -q "__imp_.*bareSymbol"; then echo "$f"; fi; done` to find all affected .o files.

---

## Fix #23: `DebugVar()` missing semicolon in `lduPrimitiveMeshAssemblyTemplates.C`

**Symptom**: Compile error during finiteVolume rebuild: `lnInclude\lduPrimitiveMeshAssemblyTemplates.C:488:35: error: expected ';' after expression`.

**Root cause**: `DebugVar(var)` on Windows expands to `((void)0)` (an expression), not a block statement. Line 488 had `DebugVar(lduAddr().size())` without a trailing semicolon. On Linux this works because `DebugVar` expands to `{...}` (block), but on Windows it needs the semicolon.

**Fix**: Added semicolon after `DebugVar(lduAddr().size())` on line 488 of `lduPrimitiveMeshAssemblyTemplates.C`. Copied fix to lnInclude.

**Lesson**: `DebugVar(x)` on Windows is just `((void)0)` - always follow it with a semicolon. Only one instance in the codebase lacked it.

---

## Fix #24: `turbulenceModel::propertiesName` cross-DLL access

**Symptom**: Crash (exit code 127) during `turbulenceModel::New()` call in simpleFoam. Traces showed turbulenceProperties IOdictionary read succeeded but the actual `New()` call crashed.

**Root cause**: `turbulenceModel::propertiesName` is a `static const word` defined in `libturbulenceModels.dll`, used as a default parameter in `IncompressibleTurbulenceModel::New()`. When called from simpleFoam.exe, the default parameter evaluates at the call site, creating a cross-DLL data access through a JMP thunk. `FOAM_EXPORT_DATA` cannot be used because it only works for libOpenFOAM.dll symbols (keyed on `FOAM_BUILDING_LIB`).

**Fix**: Passed `word("turbulenceProperties")` explicitly at the call site in `createFields.H` instead of relying on the default parameter.

**Lesson**: Default parameters that reference static data in OTHER DLLs (not libOpenFOAM) cannot use `FOAM_EXPORT_DATA`. Workaround: pass the value explicitly at the call site. A more systematic fix would be per-library export macros (e.g., `TURBULENCE_EXPORT_DATA`).

---

## Fix #25: `UPstream` static members cross-DLL access

**Symptom**: Clean error exit (code 1) with message "Unsupported communications type -1" from `GeometricBoundaryField.C` at line 611. Turbulence model selection worked fine (RAS/kEpsilon), crash came during field evaluation.

**Root cause**: `UPstream::defaultCommsType` is a static data member in libOpenFOAM.dll, used as a default parameter in `GeometricBoundaryField::evaluate(const UPstream::commsTypes commsType = UPstream::defaultCommsType)`. Template code compiled in downstream DLLs reads the JMP thunk bytes as a commsTypes enum value → gets -1 (0xFFFFFFFF from thunk instruction bytes), which is not a valid communications type.

**Fix**: Added `FOAM_EXPORT_DATA` to 7 UPstream static data members in `UPstream.H`: `commsTypeNames`, `floatTransfer`, `nProcsSimpleSum`, `nProcsNonblockingExchange`, `nPollProcInterfaces`, `defaultCommsType`, `maxCommsSize`. Required recompilation of ~217 .o files across finiteVolume and all downstream DLLs.

**Lesson**: `UPstream::defaultCommsType` is used as a default parameter in dozens of functions across the codebase. Any static data used as a default parameter in template code is a cross-DLL hazard. Apply `FOAM_EXPORT_DATA` to all frequently-accessed static members, especially those used in default parameters.

---

## Fix #26: `VectorSpace::componentNames` and pTraits static members cross-DLL access

**Symptom**: Segfault during `lduMatrix::solver::New()` in `fvMatrixSolve.C` at `pTraits<Type>::componentNames[cmpt]`. Also, `SolverPerformance<Type>::debug` returned garbage value `-1628690945` (JMP thunk bytes).

**Root cause**: `VectorSpace<Form, Cmpt, Ncmpts>` declares `static const char* const componentNames[]`, `static const Form zero/one/max/min/rootMax/rootMin` - all without `FOAM_EXPORT_DATA`. These are defined in libOpenFOAM.dll (`floatVectors.C`, etc.) but accessed from libfiniteVolume.dll's template code (`fvMatrixSolve.C`). The `componentNames` pointer is read through a JMP thunk → garbage pointer → segfault when dereferenced. Similarly, `pTraits<scalar>`, `pTraits<int32_t>`, and other pTraits specializations have static members without FOAM_EXPORT_DATA.

**Fix**: Added `FOAM_EXPORT_DATA` to all static data members in:
- `VectorSpace.H`: `typeName`, `componentNames[]`, `zero`, `one`, `max`, `min`, `rootMax`, `rootMin`
- `Scalar.H`: all pTraits<Scalar> static members (typeName, componentNames, zero, one, max, min, rootMax, rootMin, vsmall)
- `bool.H`, `char.H`, `int8.H`, `int32.H`, `int64.H`, `uint8.H`, `uint32.H`, `uint64.H`, `complex.H`: all pTraits static members (61 total across all files)

Required full recompilation of all downstream DLLs (VectorSpace.H is included transitively by nearly every OpenFOAM header).

**Lesson**: ANY static data member in a template class whose explicit instantiation lives in libOpenFOAM.dll needs `FOAM_EXPORT_DATA` if it's accessed from downstream DLLs. This includes `VectorSpace` and all `pTraits` specializations. The `FOAM_EXPORT_DATA` macro works correctly for these because `FOAM_BUILDING_LIB` is only defined when building libOpenFOAM.dll, and downstream DLLs don't define new VectorSpace or pTraits types.

---

## Fix #27: VectorSpace template static data members need explicit specialization declarations for dllimport

**Symptom**: Even with `FOAM_EXPORT_DATA` on `VectorSpace::componentNames`, `VectorSpace::zero`, etc. (Fix #26), downstream .o files (e.g., `fvMesh.o` in libfiniteVolume.dll) still compiled WITHOUT `__imp_` prefixed references to these members. The `__declspec(dllimport)` attribute was silently ignored for template class static members.

**Root cause**: Clang/MSVC ignores `__declspec(dllimport)` on static data members of template classes for implicit template instantiations. When the compiler implicitly instantiates `VectorSpace<Vector<double>, double, 3>`, it does NOT honor the `dllimport` attribute on the static members. This was confirmed by:
- `llvm-nm fvMesh.o | grep componentNames` → NO `__imp_` prefix (plain symbol reference)
- `llvm-nm fvMesh.o | grep FatalError` → HAS `__imp_` prefix (non-template static, works correctly)

**Investigation**: Two approaches were tried:

1. **`extern template class VectorSpace<...>`**: This tells the compiler that ALL members (functions + data) come from an external library. It successfully generated `__imp_` for data members, BUT also suppressed non-inline function instantiation (Istream constructor, `operator>>`, `operator<<`, `name()` from VectorSpace.C). This caused LNK2001 (unresolved external) errors in ~20 .o files.

2. **Explicit specialization declarations** (WINNER): Declare each data member individually as an explicit specialization with `FOAM_EXPORT_DATA`:
   ```cpp
   template<> FOAM_EXPORT_DATA const char* const
       Foam::VectorSpace<Form, Cmpt, N>::typeName;
   template<> FOAM_EXPORT_DATA const char* const
       Foam::VectorSpace<Form, Cmpt, N>::componentNames[];
   template<> FOAM_EXPORT_DATA const Form
       Foam::VectorSpace<Form, Cmpt, N>::zero;
   // ... etc for one, max, min, rootMax, rootMin
   ```
   This tells the compiler that these specific data members are externally defined with dllimport, WITHOUT suppressing function instantiation.

**Fix**: Created `FOAM_VECTORSPACE_EXTERN_DATA(Form, Cmpt, N)` macro that declares all 8 VectorSpace static data members (typeName, componentNames, zero, one, max, min, rootMax, rootMin) as explicit specializations with `FOAM_EXPORT_DATA`. Applied to:
- `vector.H`: `FOAM_VECTORSPACE_EXTERN_DATA(Foam::Vector<float>, float, 3)` and `(Foam::Vector<double>, double, 3)`
- `tensor.H`: `FOAM_VECTORSPACE_EXTERN_DATA(Foam::Tensor<float>, float, 9)` and `(Foam::Tensor<double>, double, 9)`
- `symmTensor.H`: `FOAM_VECTORSPACE_EXTERN_DATA(Foam::SymmTensor<float>, float, 6)` and `(Foam::SymmTensor<double>, double, 6)`
- `sphericalTensor.H`: `FOAM_VECTORSPACE_EXTERN_DATA(Foam::SphericalTensor<float>, float, 1)` and `(Foam::SphericalTensor<double>, double, 1)`

The macro block is guarded by `#if defined(_WIN32) && !defined(FOAM_BUILDING_LIB)`, so libOpenFOAM sees no change (uses `dllexport` from the class definition), while downstream DLLs get proper `dllimport` for these specific data members.

**Verification**: After applying, `llvm-nm fvMesh.o | grep componentNames` showed ALL references with `__imp_` prefix. `libfiniteVolume.dll` linked without errors (no suppression of function instantiation).

**Status**: Headers modified and copied to lnInclude. Full recompilation of libfiniteVolume.dll needed (all ~428 .o files must be rebuilt to pick up the new dllimport declarations).

**Lesson**: `__declspec(dllimport)` on template class static data members is IGNORED by Clang/MSVC for implicit instantiations. The workaround is explicit specialization declarations for each data member, which forces the compiler to use dllimport. `extern template class` is too aggressive (suppresses functions too). The explicit specialization declaration approach is the correct middle ground: it generates `__imp_` for data without affecting functions.

---

## Fix #28: SYCL reductionSum returns 0 on AdaptiveCpp OMP backend

**Symptom**: simpleFoam runs a full SIMPLE iteration but all solver residuals report `Initial residual = 0, Final residual = 0, No Iterations 0`. The `gSum()` function returns 0 for all fields. The `weightedAverage()` function returns 0, causing `continuity errors` to report 0. The solver converges in one iteration with no actual computation.

**Root cause**: `syclExecutor::_backendReductionSum()` in `syclExecutor.cpp` used `sycl::buffer` + `sycl::reduction` + `sycl::parallel_for` to compute sums. On the AdaptiveCpp OMP backend (which was "built without OpenMP support" per the runtime warning), this SYCL reduction pattern always returns 0. The SYCL runtime silently produces zero instead of an error.

Note: `_backendReductionCompare()` (used by min/max) already had a CPU fallback loop, and `_backendFor()` (parallel_for without reduction) works correctly in sequential mode. Only the reduction sum path was broken.

**Impact scope**: `syclExecutor.cpp` is a `#ifdef NoRepository` template included from `syclExecutor.H`. Every `.o` file that uses reductions gets its own compiled copy. Analysis found ~940 .o files across all libraries contain `_backendReductionSum`:
- libOpenFOAM: 80/612 files
- libfiniteVolume: 357/428 files
- libmeshTools: 95/262 files
- libturbulenceModels: 51/56 files
- Plus all other downstream DLLs

Fixing the source requires recompiling ALL affected .o files across ALL libraries (~6+ hours).

**Fix**: Changed `_backendReductionSum()` in `syclExecutor.cpp` to use a CPU loop fallback (matching the pattern already used by `_backendReductionCompare()`):
```cpp
resultT localSum = Foam::Zero;
for (label i = 0; i < size; ++i)
{
    localSum += lambda(i);
}
*result += localSum;
```
Also applied a targeted workaround in `DimensionedField::weightedAverage()` to use a CPU loop instead of `gSum()`, which takes effect without rebuilding all downstream DLLs.

**Diagnostic technique**: The key insight was comparing `_backendReductionSum` (broken) vs `_backendReductionCompare` (working). Both are in the same file; the difference was that reductionCompare already had a CPU fallback while reductionSum used SYCL buffers. The `AdaptiveCpp Warning: Kernel launcher was built without OpenMP support` message in stderr was the clue.

**Lesson**: When using SYCL with fallback backends (like AdaptiveCpp's OMP backend), test each SYCL feature individually. `sycl::parallel_for` may work correctly in sequential mode while `sycl::reduction` silently returns wrong results. Always provide CPU fallback paths for reduction operations. Template code in NoRepository headers means a source-level fix requires rebuilding every downstream library.

---

## Fix #28: SYCL reductionSum returns 0 on OMP backend

**Symptom**: `simpleFoam` solver diverged or produced wrong results. `DimensionedField::weightedAverage` returned 0 because `_backendReductionSum` returned 0 on the OMP (CPU) backend.

**Root cause**: The SYCL reduction kernel in `syclExecutor.cpp` returns 0 on the OpenMP/CPU backend (OMP). The `sycl::reduction` with `sycl::plus<>` doesn't produce correct results when running on the CPU fallback.

**Fix**: Added CPU fallback path in `syclExecutor.cpp::_backendReductionSum` that uses a sequential loop when the OMP backend is detected. Also added CPU workaround in `DimensionedField.C::weightedAverage` to compute the sum sequentially.

**Lesson**: SYCL backends are not equally functional. Always test reductions on each backend (CUDA, OMP) independently. ~940 .o files need rebuilding for this fix to take effect in all DLLs.

---

## Fix #29: DEF file missing static library symbols (exit 127)

**Symptom**: `simpleFoam.exe` fails with exit code 127 (DLL load failure) after regenerating `libOpenFOAM.def`.

**Root cause**: `generate-msvc-def` was called with only `@link_objects.rsp` (the .o files). Static libraries (`libOSspecific.lib`, `libPstream_static.lib`) that are linked into `libOpenFOAM.dll` were not included. This caused 45 symbols (Pstream functions, OS functions like `exists`, `isDir`, `mkDir`) to be missing from the DEF file. Downstream DLLs that import these symbols couldn't load.

**Fix**: Include static libraries when regenerating the DEF file:
```bash
generate-msvc-def libOpenFOAM.def @link_objects.rsp libOSspecific.lib libPstream_static.lib
```
Symbol count went from 24,947 to 25,283.

**Lesson**: The wmake makefile passes static libs via `$(filter %.lib,$(PROJECT_LIBS) $(LIB_LIBS))`. When manually regenerating DEF files, always include the static .lib files too.

---

## Fix #30: writeHeader crashes on type() virtual dispatch

**Symptom**: `simpleFoam` crashes with SIGSEGV during `runTime.write()` when writing field objects (e.g., `nut`, `U`, `p`, `cumulativeContErr`). The crash occurs in `IOobject::writeHeader(Ostream& os)` at `this->type()`.

**Root cause**: The `TypeName` macro defines `virtual const word& type() const { return typeName; }` as an inline COMDAT function. When multiple DLLs instantiate the same template (e.g., `GeometricField<scalar,...>`), each gets a COMDAT copy of `type()`. The linker picks ONE copy via COMDAT folding — if it picks a copy from a different DLL than where `typeName` is defined, the access goes through a DEF-file JMP thunk, reading instruction bytes as string data → SIGSEGV.

**Key insight**: COMDAT folding of `type()` makes cross-DLL data access unpredictable. Even types whose `typeName` is in libOpenFOAM (like `UniformDimensionedField<scalar>`) can crash if the linker picks a `type()` copy from libfiniteVolume.

**Fix**: Three-tier approach in `IOobject::writeHeader(Ostream& os)`:
1. Use `headerClassName()` if set (e.g., from file read) — this is always safe
2. On Windows, use RTTI (`typeid(*this).name()`) to detect known-problematic template types: `GeometricField`, `UniformDimensionedField`, `DimensionedField`. For these, use `this->name()` as fallback.
3. For all other types (non-template, or safe templates like `IOField`), call `this->type()` normally.

**Also**: `GeometricField::readFields()` propagates `headerClassName` from the temporary reader to the actual object, so fields read from file always use path #1.

**Status**: WORKING. simpleFoam runs 10+ iterations on pitzDaily, writes correct results. Output file class names use object name instead of type name for programmatic template objects (e.g., `class p;` instead of `class volScalarField;`), but this is tolerable — files can be read back correctly.

**Lesson**: On Windows DLLs with DEF-file exports, COMDAT-folded virtual functions that access static data members are inherently unsafe. Use RTTI-based type detection as a safe alternative when `type()` cannot be called.

---

## Fix #31: Exit-time crash during static destruction

**Symptom**: `simpleFoam.exe` prints "End" and produces all correct output files, then crashes with exit code 139 (SIGSEGV) during C++ destructor/cleanup chains after `main()` returns. `blockMesh.exe` does NOT have this issue.

**Root cause**: After `main()` returns, C++ destroys local variables (fields, mesh, Time), then static objects across DLLs. Template-heavy executables like simpleFoam have many GeometricField destructors that access cross-DLL data via the same COMDAT/DEF-thunk mechanism as Fix #30.

**Fix**: Added `std::_Exit(0)` before `return 0;` in `simpleFoam.C` (guarded by `#ifdef _WIN32`). This calls exit handlers and flushes I/O but skips C++ destructor chains. All output files are already written and flushed at this point.

**Status**: RESOLVED. simpleFoam exits with code 0.

**Lesson**: On Windows with multiple DLLs, use `std::_Exit()` or `_exit()` to avoid destructor-chain crashes at program exit. This is safe when all I/O is complete. Apply to other solver executables as needed.

---

## Fix #32: RTTI-based writeHeader class name on Windows

**Symptom**: Output files written by simpleFoam had wrong class names. Fields created programmatically (phi, cumulativeContErr) or read by stale DLLs (k, epsilon, nut) got their object name or typedef name as the class, not the OpenFOAM typeName. This caused:
1. Non-fatal warnings on restart: `Unexpected class name "surfaceScalarField" expected "GeometricField"`
2. Fatal errors on restart: `unexpected class name uniformDimensionedScalarField expected UniformDimensionedField`

**Root cause**: Two interacting issues:
1. **COMDAT stale copies**: Template functions (e.g., `readFields()`) are compiled into each DLL separately. DLLs compiled before headerClassName propagation was added (Fix #30) have stale copies that don't set headerClassName. Fields read by these DLLs get empty headerClassName.
2. **`type()` crash on Windows**: The virtual `type()` function accesses the static `typeName` member which may be in a different DLL. Due to COMDAT folding and DEF-file JMP thunks, this can crash (same mechanism as Fix #30).

When headerClassName is empty AND `type()` crashes, `writeHeader()` has no way to determine the correct class name.

**Fix**: In `IOobjectWriteHeader.C`, added RTTI-based class identification using `typeid(*this).name()` (MSVC RTTI). The mapper returns the OpenFOAM **typeName** (not the typedef name):
- `GeometricField<...>` → `"GeometricField"` (not `"volScalarField"`)
- `UniformDimensionedField<...>` → `"UniformDimensionedField"` (not `"uniformDimensionedScalarField"`)
- `DimensionedField<...>` → `"DimensionedField"`

The typeName is what `readStream()` checks against, so using it ensures files can be read back without errors.

**Key insight**: The initial approach (returning typedef names like "volScalarField") caused warnings for GeometricField reads and **fatal errors** for UniformDimensionedField reads. The typedef names don't match the typeName that `readStream()` uses for validation.

**Status**: RESOLVED. simpleFoam runs to convergence (273 iterations), writes correct class names, restarts cleanly from latestTime, zero warnings, zero fatal errors.

**Lesson**: When bypassing `type()` on Windows, always return the OpenFOAM **typeName** (the string from the `TypeName()` macro), not typedef aliases. The typeName is what internal consistency checks validate against.

---

## Fix #33: meshObject::debug cross-DLL access in NoRepository template crashes mesh destructor

**Symptom**: `simpleFoam.exe` segfaults (exit 139) during exit-time mesh destruction. Crash happens inside `fvMesh::clearOut()` → `meshObject::clearUpto()` template. SEH handler shows: access violation reading address `0xFFFFFFFFFFFFFFFF` at `operator<<(Ostream&, const char*)` in libOpenFOAM.dll (offset `0x109D7B`).

**Root cause**: `MeshObject.C` is a NoRepository template file (`#ifdef NoRepository #include "MeshObject.C"`), so its template code is compiled into every downstream DLL (e.g., libfiniteVolume.dll). The `meshObject::clearUpto()` template contains `if (meshObject::debug) { Pout << ... }` debug output blocks. Both `meshObject::debug` and `Pout` are data symbols defined in libOpenFOAM.dll.

When the template is compiled into libfiniteVolume.dll, these data accesses go through DEF file thunks (JMP stubs). `meshObject::debug` is declared with `FOAM_TYPENAME_EXPORT` which is always `__declspec(dllexport)` on Windows — it never uses `dllimport`, so the compiler never generates proper `__imp_` references. The DEF thunk for `meshObject::debug` returns garbage (instruction bytes read as int), which evaluates as non-zero, causing the code to enter the `if` block. Then `Pout` is accessed via another DEF thunk, yielding a corrupt vtable pointer. The `operator<<(Ostream&, const char*)` call dispatches through `callq *0x70(%rax)` with `%rax` pointing to garbage → segfault reading `0xFFFFFFFFFFFFFFFF`.

**Fix**: Two-part fix:
1. **Source fix** (MeshObject.C): Added `MESHOBJECT_DEBUG` macro that evaluates to `false` on Windows, replacing all `if (meshObject::debug)` checks. This is the same approach as the existing `DebugInFunction` macro (Fix #20). The compiler optimizes away the entire debug block, avoiding both the `meshObject::debug` and `Pout` cross-DLL accesses.
2. **Solver workaround** (simpleFoam.C): Until libfiniteVolume.dll is rebuilt with the fix, `meshPtr.release()` leaks the mesh instead of destructing it. Scoped destruction analysis confirmed: fields destruct OK, simpleControl destructs OK, runTime destructs OK — only the mesh destructor crashes.

**Files changed**:
- `src/OpenFOAM/meshes/MeshObject/MeshObject.C` - Added `MESHOBJECT_DEBUG` macro, replaced 15 occurrences
- `src/OpenFOAM/lnInclude/MeshObject.C` - Copy of above
- `applications/solvers/incompressible/simpleFoam/simpleFoam.C` - `meshPtr.release()` workaround

**Lesson**: ALL NoRepository template code is compiled into downstream DLLs. Any access to static data members (debug, typeName) or global variables (Pout, Info) from template bodies is a cross-DLL data access. `FOAM_TYPENAME_EXPORT` (always `dllexport`) does NOT provide proper `dllimport` for consumers — the linker uses DEF thunks which corrupt data reads. Debug output in templates must be disabled on Windows via compile-time guards, not runtime checks on cross-DLL data.

**Diagnostic technique**: Used `SetUnhandledExceptionFilter()` with a custom SEH handler to capture crash address, fault type, and module bases without a debugger. Cross-referenced crash offset (`0x109D7B` from libOpenFOAM base) with `llvm-objdump -d --start-address=0x180109D7B` to identify the crashing instruction (`callq *0x70(%rax)` in `operator<<(Ostream&, const char*)`).

---

## Fix #34: FOAM_EXPORT_DATA on Pout/Perr/Sout/Serr/Sin + typeName_() in templates

**Symptom**: Comprehensive audit of NoRepository template files revealed ~50+ potential crash sites where `Pout`, `Perr`, `Sout`, `Serr`, or `Sin` are accessed from template code compiled into downstream DLLs. These are behind `if (debug)` guards (already disabled on Windows via Fix #20/#33), but the global stream objects themselves lack `FOAM_EXPORT_DATA`, making any future access a corrupt-vtable crash. Additionally, `Type::typeName` (static data) is used in live (non-debug) code paths in MeshObject.C constructor and `New()` method.

**Root cause**: `IOstreams.H` declared `Pout`, `Perr`, `Sout`, `Serr`, `Sin` as plain `extern` without `FOAM_EXPORT_DATA`. When accessed from downstream DLLs, the DEF thunk returns garbage (instruction bytes read as data → corrupt vtable). `Info` already had `FOAM_EXPORT_DATA` and was safe. In MeshObject.C, `Type::typeName` in the constructor (line 48) and `getObjectPtr` lookup (line 77) are live code paths that access cross-DLL static data through DEF thunks.

**Fix**:
1. Added `FOAM_EXPORT_DATA` to all 5 stream globals in `src/OpenFOAM/db/IOstreams/IOstreams.H` (Sin, Sout, Serr, Pout, Perr). This provides proper `__declspec(dllimport)` in downstream DLLs.
2. Changed `Type::typeName` → `Type::typeName_()` in `MeshObject.C` lines 48 and 77 (constructor and New() method). The `typeName_()` function returns a `const char*` literal, avoiding cross-DLL data access entirely.
3. Changed `ZoneType::typeName` → `ZoneType::typeName_()` in `ZoneMesh.C` line 1142 (verbose output in `operator()` method).

**Files changed**:
- `src/OpenFOAM/db/IOstreams/IOstreams.H` - Added `FOAM_EXPORT_DATA` to Sin, Sout, Serr, Pout, Perr
- `src/OpenFOAM/meshes/MeshObject/MeshObject.C` - `Type::typeName` → `Type::typeName_()` in constructor and New()
- `src/OpenFOAM/meshes/polyMesh/zones/ZoneMesh/ZoneMesh.C` - `ZoneType::typeName` → `ZoneType::typeName_()`
- All three files copied to `src/OpenFOAM/lnInclude/`

**Lesson**: All `extern` global data in libOpenFOAM headers must have `FOAM_EXPORT_DATA` if there is ANY chance it could be accessed from downstream DLLs. For template code (NoRepository pattern), prefer function-based accessors (`typeName_()`) over static data members (`typeName`) since the function call goes through a JMP thunk (which works correctly for functions) while data access through a JMP thunk returns garbage.

---

## Fix #35: debug_() function accessor for cross-DLL safe debug switch access

**Symptom**: Fix #33 disabled ALL debug output on Windows by defining `MESHOBJECT_DEBUG false`. This was a blunt workaround — it eliminated crash risk but also eliminated the ability to debug MeshObject operations on Windows. The root cause (cross-DLL static data access through DEF thunks) applies to ALL `::debug` accesses from NoRepository template code.

**Root cause**: The `ClassName` macro declares `static int debug` with `FOAM_TYPENAME_EXPORT` (always `__declspec(dllexport)`). This means downstream DLLs never get `__declspec(dllimport)` for `debug`, so they access it through a DEF thunk. Data reads through DEF thunks return garbage (instruction bytes). There was no function-based accessor for `debug` like there was for `typeName` (`typeName_()` returns a `const char*` literal).

**Fix**: Added `debug_()` static member function pattern, mirroring `typeName_()`:
1. `className.H`: Added `static int debug_()` declaration to the `ClassName` macro (after `static int debug`)
2. `defineDebugSwitch.H`: Added `defineDebugFunction(Type)` macro → `int Type::debug_() { return Type::debug; }`, and `defineTemplateDebugFunction(Type)` → `template<> int Type::debug_() { return Type::debug; }`. Both are called from `defineDebugSwitch` and `defineTemplateDebugSwitchWithName` respectively.
3. `MeshObject.C`: Changed `MESHOBJECT_DEBUG` from `false` to `meshObject::debug_()` on Windows, **restoring debug output capability**.

When template code calls `SomeClass::debug_()` from a downstream DLL:
- The function call goes through the DEF thunk JMP → lands in the source DLL
- The function body reads `debug` locally within its own DLL (no cross-DLL data access)
- Returns the runtime debug value correctly

**Files changed**:
- `src/OpenFOAM/db/typeInfo/className.H` — `ClassName` macro: added `static int debug_()`
- `src/OpenFOAM/global/debug/defineDebugSwitch.H` — Added `defineDebugFunction`/`defineTemplateDebugFunction` macros, integrated into `defineDebugSwitch`/`defineTemplateDebugSwitchWithName`
- `src/OpenFOAM/meshes/MeshObject/MeshObject.C` — `MESHOBJECT_DEBUG` = `meshObject::debug_()` on Windows
- All copied to `src/OpenFOAM/lnInclude/`

**Lesson**: For every static data member that might be accessed from NoRepository template code, provide a function accessor (`foo_()` returning the value). Functions go through DEF thunk JMP stubs correctly; data goes through DEF thunk JMP stubs and returns garbage. The accessor must be **non-inline** (defined in the source DLL's .C file via the define macro) so the function body lives in the DLL that owns the data. This is a general architectural principle: `typeName → typeName_()`, `debug → debug_()`.

---

## Fix #34: NamespaceName macro missing debug_() declaration

**Symptom**: `finiteArea` library fails to compile with `defineTypeNameAndDebug(fa, 0)`:
```
fa.C:34:1: error: out-of-line definition of 'debug_' does not match any declaration in namespace 'Foam::fa'
```

**Root cause**: The `NamespaceName` macro in `className.H` declares `extern int debug` but NOT `int debug_()`. However, `defineTypeNameAndDebug` calls `defineDebugFunction(Type)` which expands to `int Type::debug_() { return Type::debug; }`. For namespaces, there's no matching declaration of `debug_()`. The `ClassName` macro correctly declares `static int debug_()` for classes, but the equivalent was missing for `NamespaceName`.

This was latent since `defineDebugFunction` was added in commit c4e8221 — previously built namespace objects (like `fv.o` from Feb 18) predate that commit and don't contain `debug_()`.

**Fix**: Added `int debug_()` declaration to the `NamespaceName` macro in `className.H`:
```cpp
#define NamespaceName(TypeNameString)                                          \
    NamespaceNameNoDebug(TypeNameString);                                      \
    extern int debug;                                                         \
    int debug_()
```
Also copied to `src/OpenFOAM/lnInclude/className.H`.

**Lesson**: When adding new members to definition macros (like `defineDebugFunction`), check ALL corresponding declaration macros — both class (`ClassName`) and namespace (`NamespaceName`) variants. The namespace variant is easily overlooked.

## Fix #35: objectRegistryTemplates.C typeName_() breaks non-TypeName types

**Symptom**: `snappyHexMesh` library fails to compile:
```
objectRegistryTemplates.C:619: error: no member named 'typeName_' in 'Foam::Field<double>'
```
Triggered by `lookupObject<scalarField>()` in `medialAxisMeshMover.C`.

**Root cause**: In a previous commit (27954042), `Type::typeName` was changed to `Type::typeName_()` in `objectRegistryTemplates.C` error message paths for cross-DLL safety. But container types like `Field<T>` don't have a `TypeName` macro and therefore don't have `typeName_()`. The template is instantiated with these types, causing compilation failure.

**Fix**: Reverted the two `Type::typeName_()` calls back to `Type::typeName` in the error message paths at lines 619 and 632. These are fatal error paths that always call `exit()`, so cross-DLL data access is not a concern.

**Lesson**: Template code that uses `typeName_()` can only be used with types that have the `ClassName`/`TypeName` macro. Error message paths in generic templates should use `Type::typeName` (the data member) since all TypeName types have it, and for types without it, the compiler gives a clear error at the actual instantiation site.

## Fix #36: dummyThirdParty stubs need lnInclude from real libraries

**Symptom**: All 4 dummyThirdParty decomposition stubs fail:
```
dummyScotchDecomp.cxx:29:10: fatal error: 'scotchDecomp.H' file not found
```

**Root cause**: The dummy stubs include headers from the real decomposition libraries via `-I$(LIB_SRC)/parallel/decompose/scotchDecomp/lnInclude`. These lnInclude directories only get created when the real libraries are built (via `wmakeLnInclude`). Since the real libraries are skipped (no scotch/metis/kahip installed), the lnInclude dirs don't exist.

**Fix**: Run `AllwmakeLnInclude` from `src/parallel/decompose/` before building dummyThirdParty stubs. This creates the lnInclude directories with header copies. Added to rebuild-failed.sh and rebuild-all.sh.

**Lesson**: Dummy stub libraries that include headers from their "real" counterparts need those lnInclude directories pre-created. The `AllwmakeLnInclude` script in the parent directory handles this.

## Fix #37: Guard MemoryPool::getInstance() and FatalErrorInFunction in SYCL device pass

**Symptom**: `libdecompose.dll` failed to compile with `ptxas fatal: Unresolved extern function` — first `MemoryPool::getInstance()`, then `Foam::error::operator()` from `FatalErrorInFunction`.

**Root cause**: acpp compiles ALL `.C` files for both host and device passes when `--acpp-targets=cuda:sm_86` is set. Template code in `List.C`, `ListI.H`, `UListI.H`, and `UList.C` contains `MemoryPool::getInstance()` calls and `FatalErrorInFunction` error handling. These are host-only functions that ptxas cannot resolve in device code.

**Fix**: Wrapped all `MemoryPool::getInstance()` call blocks AND all `FatalErrorInFunction` blocks in the 4 List/UList template files with `#ifndef SYCL_DEVICE_ONLY` / `#endif`. The device pass falls through to the standard `new[]`/`delete[]`/`std::copy` paths. Files modified:
- `src/OpenFOAM/containers/Lists/List/ListI.H` — doAlloc(), clear(), push_back()
- `src/OpenFOAM/containers/Lists/List/List.C` — doResize(), ~List(), transfer(), 3 constructors
- `src/OpenFOAM/containers/Lists/List/UListI.H` — fill_uniform(T), fill_uniform(zero)
- `src/OpenFOAM/containers/Lists/List/UList.C` — deepCopy() (2 overloads), byteSize()
- All 4 files copied to `src/OpenFOAM/lnInclude/`

**Unblocked**: libdecompose + snappyHexMesh, decomposePar, redistributePar, renumberMesh, mapFields, surfaceRedistributePar (7 targets).

**Lesson**: On Windows CUDA, the device pass is strict about unresolved symbols. Any host-only function called from template code needs `#ifndef SYCL_DEVICE_ONLY` guards. This includes both MemoryPool operations AND error handling paths (FatalErrorInFunction, abort(FatalError)).

---

## Fix #38: RTS table pointer accessor `()` for function-based access

**Symptom**: `createBaffles` and `surfacePatch` failed to link with DEF thunk issues on `dictionaryConstructorTablePtr_`.

**Root cause**: On Windows, `dictionaryConstructorTablePtr_` is a function (returns pointer reference) not a bare pointer variable. Code using `*dictionaryConstructorTablePtr_` or `dictionaryConstructorTablePtr_->` without `()` fails because it tries to dereference/access a function instead of calling it.

**Fix**: Changed bare pointer access to function call syntax in 9 files:
- `*dictionaryConstructorTablePtr_` → `*dictionaryConstructorTablePtr_()`
- `dictionaryConstructorTablePtr_->` → `dictionaryConstructorTablePtr_()->`
- `*dictConstructorTablePtr_` → `*dictConstructorTablePtr_()`

Files: faceSelection.C, searchableSurfaceModifier.C, helpTypeNew.C, helpBoundaryTemplates.C, addToolOption.H, implicitFunction.C, tabulatedWallFunctionNew.C, surfaceFeaturesExtraction.C.

**Unblocked**: createBaffles, surfacePatch, + all Fix #40 sub-libraries (foamHelp, setAlphaField, wallFunctionTable, surfaceFeatureExtract).

**Lesson**: ALL uses of `*XXXConstructorTablePtr_` and `XXXConstructorTablePtr_->` throughout the codebase need the `()` suffix on Windows. Search for these patterns when adding new utilities.

---

## Fix #39: FlexLexer.h include path isolation

**Symptom**: Flex-based utilities (ansysToFoam, fluent3DMeshToFoam, fluentMeshToFoam, gambitToFoam, chemkinToFoam) failed because `#include <FlexLexer.h>` couldn't find the header.

**Root cause**: FlexLexer.h exists at `/c/msys64/usr/include/FlexLexer.h` but acpp doesn't search this path. Adding `-I/usr/include` caused MSYS2's `stdlib.h` to conflict with MSVC's `cstdlib` during the CUDA device pass.

**Fix**: Copied FlexLexer.h to `wmake/include/FlexLexer.h` (isolated directory with only this header). Changed all 5 Make/options files to use `-I$(WM_DIR)/include` instead of `-I/usr/include`.

**Unblocked**: 5 flex-based utilities.

**Lesson**: Never add MSYS2 system include paths (`/usr/include`) to MSVC/CUDA builds — they conflict with MSVC C runtime headers. Copy individual headers to an isolated directory instead.

---

## Fix #40: Build sub-libraries before utility apps

**Symptom**: foamHelp, setAlphaField, wallFunctionTable, surfaceFeatureExtract failed because their sub-libraries weren't built.

**Root cause**: These utilities have sub-libraries (helpTypes, alphaFieldFunctions, tabulatedWallFunction, extractionMethod) that are normally built by their `Allwmake` scripts. Our rebuild script called `wmake` directly, skipping the sub-library build step.

**Fix**: Added `wmake libso` calls for each sub-library before building the parent app in `rebuild-remaining.sh`. Also required Fix #38 (RTS accessor) for the sub-library source files.

**Unblocked**: 4 utility apps + 4 sub-libraries.

**Lesson**: When building utilities outside of the full `Allwmake` flow, check for sub-directories with their own `Make/files` that produce `LIB =` targets. Build these before the parent app.

---

## Fix #41: Root cause analysis — MemoryPool in CUDA device code (Fix #37 deep dive)

**Symptom**: When building the `decompose` library with `--acpp-targets=cuda:sm_86`, ptxas reported `Unresolved extern function '_ZN4Foam10MemoryPool11getInstanceEv'`. Specifically, compiling `lagrangianFieldDecomposerCache.C` produced "363 warnings when compiling for sm_86" followed by the ptxas fatal error.

**Root cause**: The issue is triggered by **nested Field types** (e.g., `Field<Field<tensor>>` from `CompactIOField`) whose mapping constructor uses `parallelFor` on element types that have non-trivial assignment operators.

The precise chain:

1. `CompactIOField<Field<tensor>, tensor>` inherits from `Field<Field<tensor>>`.
2. `lagrangianFieldDecomposerTemplates.C` (included via NoRepository) calls the mapping constructor: `Field<Field<tensor>>(field, particleIndices_)`.
3. The mapping constructor calls `Field::map(mapF, mapAddressing)` (Field.C:310).
4. `map()` uses `parallelFor` with a kernel lambda that captures **only raw pointers**:
   ```cpp
   auto fPtr = f.begin();         // Field<tensor>*
   auto mapFPtr = mapF.cbegin();  // const Field<tensor>*
   auto mapAddr = [=](label i) {
       fPtr[i] = mapFPtr[mapI];   // Element assignment via raw pointer
   };
   exec.parallelFor(mapAddr, f.size());
   ```
5. The lambda only captures raw pointers — no Field/List objects. But `fPtr[i] = mapFPtr[mapI]` where elements are `Field<tensor>` invokes `Field<tensor>::operator=` → `List<tensor>::operator=` → `List::doResize()` → `MemoryPool::getInstance()`.
6. AdaptiveCpp's Clang plugin (`Frontend.hpp`) traces from the kernel through `CompleteCallSet`, following:
   - `VisitCallExpr`: direct function calls
   - `VisitCXXConstructExpr`: constructors and destructors
   - Template instantiations (`shouldVisitTemplateInstantiations() = true`)
   - Implicit code (`shouldVisitImplicitCode() = true`)
7. The trace follows: kernel → lambda `operator()` → `operator=` on `Field<tensor>` (pointer subscript returns a reference to a complex type) → `List::operator=` → `doResize` → `MemoryPool::getInstance()`.
8. ALL traced functions get `CUDAHostAttr + CUDADeviceAttr` and are re-emitted in the device pass.
9. `MemoryPool::getInstance()` is defined in `MemoryPool.C` (host-only) → ptxas error.

**Key insight**: The kernel lambda captures only raw pointers, but the **element type's operators** (accessed through those pointers) cascade into List allocation methods. For simple types (scalar, vector, tensor), element assignment is a trivial `FOAM_DEVICE_I` operation. For nested Field types, it's a full C++ assignment involving memory allocation.

**Cineca already knew about this for CUDA/HIP** (see `lagrangianFieldDecomposerTemplates.C` lines 85-100):
```cpp
#if defined(have_cuda) || defined(have_hip)
    Field<Field<Type>>()   // avoids mapping constructor + parallelFor on nested types
#else
    Field<Field<Type>>(field, particleIndices_)  // uses mapping constructor → parallelFor
#endif
```
But they didn't add `have_sycl` to the guard. When building SYCL+CUDA via acpp, `have_sycl` is defined but `have_cuda` is NOT, so the unguarded path is taken.

**Verified empirically**:
- Test with `Field<scalar>` + `negate()` + `operator+=`: compiles cleanly, no ptxas error (element ops are trivial).
- Test with `Field<Field<tensor>>` mapping constructor: compiles cleanly only because Fix #37 guards are already in place.
- Test with no kernel code (no `parallelFor` calls): device pass doesn't even run — no functions to mark.
- `-foffload-implicit-host-device-templates` NOT on by default in this Clang build — confirmed by test (not the mechanism).

**Why the plugin marking cannot be disabled**: The `CompleteCallSet` tracing is fundamental to SYCL kernel compilation — without it, the actual kernel body wouldn't be compiled for device. The trace is correct; the problem is that nested types have operators that cascade into host-only code.

**Fix**: `#ifndef SYCL_DEVICE_ONLY` guards around MemoryPool and FatalErrorInFunction blocks in List/UList templates (Fix #37). `SYCL_DEVICE_ONLY` is defined by AdaptiveCpp during the CUDA device pass (`cuda_backend.hpp` line 42 → `backend.hpp` line 60-62), so the guards correctly strip host-only code from the device compilation, letting List methods compile for device using fallback paths (`new[]`/`delete[]` instead of pool, no error checking).

**Additional recommended fix**: Add `have_sycl` to the existing guards in `lagrangianFieldDecomposerTemplates.C`:
```cpp
#if defined(have_cuda) || defined(have_hip) || defined(have_sycl)
```
This avoids running `parallelFor` on nested Field types entirely, which is both safer and more efficient (no GPU dynamic allocation for nested types).

**Key files**:
- `src/parallel/decompose/decompose/lagrangianFieldDecomposerTemplates.C` — the trigger: mapping constructor on nested Field types
- `src/OpenFOAM/fields/Fields/Field/Field.C:310` — `Field::map()` with `parallelFor`
- `<acpp>/include/AdaptiveCpp/AdaptiveCpp/compiler/Frontend.hpp` — plugin AST visitor, `CompleteCallSet`, `applyAttributes()`
- `<acpp>/include/AdaptiveCpp/AdaptiveCpp/sycl/libkernel/backend.hpp` — defines `SYCL_DEVICE_ONLY`

**Lesson**: When using `parallelFor` on `Field<Type>`, the kernel lambda's element operations (`fPtr[i] = ...`) are compiled for device. If `Type` itself is a complex type with non-trivial operators (like `Field<T>`), those operators and their full transitive call graph end up in device code. The plugin's `CompleteCallSet` correctly traces this. Guard host-only code with `#ifndef SYCL_DEVICE_ONLY`, or avoid `parallelFor` on nested types (as Cineca does for CUDA/HIP). When targeting CUDA via SYCL (acpp), existing `#if defined(have_cuda)` guards need `|| defined(have_sycl)` added.

---

## Fix #41: MemoryPool::New() silently returns dummyMemoryPool instead of fixedSizeMemoryPool

**Symptom**: `simpleFoam -pool fixedSizeMemoryPool -poolSize 1` with `ACPP_VISIBILITY_MASK=cuda` crashes with `CUDA:700` (`cudaErrorIllegalAddress`) during the first GPU kernel launch. Diagnostics showed no `malloc_shared` call, meaning the pool was never actually created.

**Root cause**: Fix #2 added auto-creation of `dummyMemoryPool` in `MemoryPool::getInstance()` during DLL static init. When `MemoryPool::New("fixedSizeMemoryPool", ...)` was later called from `main()`, the `if (!instance)` check at line 73 found the dummyMemoryPool already existed and returned it instead of creating the requested fixedSizeMemoryPool. All Field data was allocated via `std::malloc()` (CPU-only heap), not `sycl::malloc_shared()` (GPU-accessible USM). GPU kernels then accessed non-USM pointers → `cudaErrorIllegalAddress`.

**Fix**: Added logic in `MemoryPool::New()` to detect and replace the auto-created dummyMemoryPool (identified by `size() == 0` and type != "dummyMemoryPool" being requested) by deleting it and setting `instance = nullptr` before proceeding with the normal creation path.

**Lesson**: When implementing fallback/auto-creation patterns for singletons, ensure the explicit creation path (`New()`) can override the auto-created instance. The auto-created dummyMemoryPool was essential for DLL static init safety (Fix #2), but it must not prevent the real pool from being created later.

---

## Fix #42: Exit-time crash with CUDA backend on Windows

**Symptom**: `simpleFoam` with CUDA backend runs to completion (prints `End`, writes all output files), but crashes with segfault (exit code 139) during process exit. Even `_exit(0)` triggers the crash because Windows DLL termination routines run CUDA cleanup code.

**Root cause**: On Windows, process termination calls `DllMain(DLL_PROCESS_DETACH)` for all loaded DLLs. The AdaptiveCpp CUDA runtime and/or CUDA driver cleanup code runs during this phase and segfaults — likely accessing already-freed USM memory or tearing down CUDA context in the wrong order.

**Fix**: Added `_exit(0)` before `return 0` in simpleFoam (guarded by `#ifdef _WIN32`). The solver output is functionally correct — all time steps computed, results written, `End` printed. The crash is purely at exit-time cleanup. Exit code from bash is 139 but the solver completed successfully.

**Status**: Workaround only. The exit-time crash does not affect correctness. True fix requires either: (a) explicit CUDA context teardown before `_exit()`, (b) AdaptiveCpp fix for Windows DLL detach, or (c) using TCC driver mode instead of WDDM.

**Lesson**: On Windows WDDM with CUDA, process exit can trigger segfaults in GPU runtime cleanup. `_exit()` is insufficient because DLL detach still runs. For production use, ignore the exit code and check `End` in stdout instead.

---

## Fix #43: CUDA backend sycl::reduction returning 0

**Symptom**: `sycl::reduction` with `sycl::buffer` returned 0 on CUDA backend via AdaptiveCpp. Solver residuals incorrectly zero.

**Root cause**: AdaptiveCpp's `reduction_engine.hpp` passed the reduction result by value instead of by reference through the lambda chain. The value never propagated back.

**Fix**: Patched AdaptiveCpp `reduction_engine.hpp` (pass-by-reference). Also replaced buffer-based reduction in `syclExecutor.cpp` with USM-based (`sycl::malloc_shared`) to avoid buffer warnings.

**Lesson**: When AdaptiveCpp reduction returns 0, check the pass-by-value/reference chain in `reduction_engine.hpp`.

---

## Fix #44: Spurious debug output from NoRepository templates on Windows

**Symptom**: simpleFoam stdout flooded with debug messages: "From New", "Constructing laplacianScheme<Type, GType>", "Matrix dominance test", "Cache: Calculating grad(U)" — 87 messages per run. These appeared even though all debug switches were set to 0.

**Root cause**: Multiple interacting causes:

1. **DEF thunk data corruption**: NoRepository template `.C` files (e.g., `laplacianScheme.C`, `gradScheme.C`) are compiled into every downstream DLL that includes their headers. These templates check `fv::debug`, `surfaceInterpolation::debug`, `solution::debug` etc. When accessed cross-DLL through DEF file thunks, the JMP instruction bytes are read as an int value (e.g., 1034561023 instead of 0) — always non-zero, so debug output always fires.

2. **Template class static `debug` members**: `surfaceInterpolationScheme<Type>::debug` is a template class static. Even with `__declspec(dllimport)`, MSVC ignores dllimport for template class statics. The `||` in `if (surfaceInterpolation::debug_() || surfaceInterpolationScheme<Type>::debug)` bypasses the safe accessor.

3. **`ddtScheme.C` class-level `debug` access**: `if (debug > 1)` uses the inherited class `debug` member, which is also corrupt through DEF thunks when accessed from downstream DLLs.

4. **Stale DLLs**: Even after fixing the template source code, ALL downstream DLLs that include NoRepository templates must be rebuilt. Missing even one (e.g., `libincompressibleTurbulenceModels.dll`) causes messages to persist.

**Fix**: Changed ~56 files across `src/finiteVolume/` and `src/OpenFOAM/`:

- `fv::debug` → `fv::debug_()` in 11 scheme `.C` files (laplacian, grad, ddt, convection, div, snGrad, limited variants, multivariate)
- `surfaceInterpolation::debug` → `surfaceInterpolation::debug_()` in 3 files
- `surfaceInterpolationScheme<Type>::debug` → `surfaceInterpolationScheme<Type>::debug_()` in surfaceInterpolationScheme.C (2 occurrences)
- `if (debug > 1)` → `if (debug_() > 1)` in ddtScheme.C (2 occurrences)
- `if (debug)` → `if (debug_())` in ~30+ finiteVolume `.C` files (fvMatrix, fvMatrixSolve, fvScalarMatrix, fvMesh, solutionControl, simpleControl, pimpleControl, MRFZone, fvOptionListTemplates, volPointInterpolation, wallDistAddressing, etc.)
- `solution::debug` → `solution::debug_()` in solutionTemplates.C (added `debug_()` to solution.H/C)
- Set `lduMatrix 0;` in etc/controlDict
- Rebuilt 6 DLLs: finiteVolume, fvOptions, turbulenceModels, incompressibleTurbulenceModels, atmosphericModels + simpleFoam
- Also removed 4 diagnostic fprintf traces from globals.C, debug.C, IOobjectReadHeader.C, IOobject.C

**Diagnostic approach**:
1. Added `fprintf(stderr, "DIAG: fv::debug_()=%d fv::debug=%d\n", fv::debug_(), fv::debug)` — confirmed DEF thunk garbage (1034561023 vs 0).
2. Hardcoded `fv::debug_() { return 0; }` — messages persisted (proving not all DLLs were rebuilt).
3. Replaced all debug checks with `if(false)` + rebuilt all downstream DLLs — confirmed messages from these exact code paths.
4. Restored `debug_()` checks + all DLLs rebuilt — clean output, 0 debug messages.

**Lesson**: When fixing cross-DLL data access in NoRepository templates, you MUST rebuild ALL downstream DLLs, not just the library that defines the template. Each DLL has its own compiled copy of the template code. Use `grep -rl "string literal" build/` to find ALL .o files containing old code, and check DLL timestamps to identify which are stale.

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
