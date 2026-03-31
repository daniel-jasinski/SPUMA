# Runtime Selection Table Fix for Windows DLLs

## The Problem

OpenFOAM's runtime selection system (used for selecting turbulence models, boundary conditions, solvers, etc.) uses static hash tables to store constructor function pointers:

```cpp
// Original pattern in runTimeSelectionTables.H
#define declareRunTimeSelectionTable(autoPtr,baseType,argNames,argList,parList) \
                                                                               \
    typedef autoPtr<baseType> (*argNames##ConstructorPtr)argList;              \
    typedef HashTable<argNames##ConstructorPtr, word, hash> argNames##ConstructorTable; \
    static argNames##ConstructorTable* argNames##ConstructorTablePtr_;         \
    // ...
```

**On Windows DLLs, this causes a critical problem:**

1. `libOpenFOAM.dll` has its own `constructorTablePtr_`
2. `libTurbulenceModels.dll` has its own separate `constructorTablePtr_`
3. `libFiniteVolume.dll` has another separate copy
4. Each DLL registers types to its **own** table
5. When code looks up a type, it searches the **wrong** (empty) table
6. Result: "Unknown type" errors even though the type was registered

## The Solution: Function-Based Table Access

Instead of exposing the table as a static member variable, expose it through a **function** that returns a reference:

```cpp
// BEFORE (problematic):
static ConstructorTable* constructorTablePtr_;

// AFTER (works across DLLs):
static ConstructorTable*& constructorTablePtr_()
{
    static ConstructorTable* ptr = nullptr;
    return ptr;
}
```

### Why This Works

1. **Function-local statics** in C++11+ are guaranteed to be:
   - Initialized exactly once (thread-safe initialization)
   - The same instance regardless of which DLL calls the function

2. When `libTurbulenceModels.dll` calls `constructorTablePtr_()`, it gets the **same** pointer reference as when `simpleFoam.exe` calls it

3. All registrations go to the **single shared table**

## Modified Macros

### runTimeSelectionTables.H

The `declareRunTimeSelectionTable` macro changes from:

```cpp
// BEFORE:
#define declareRunTimeSelectionTable(autoPtr,baseType,argNames,argList,parList) \
    typedef HashTable<...> argNames##ConstructorTable;                          \
    static argNames##ConstructorTable* argNames##ConstructorTablePtr_;          \
    static void construct##argNames##ConstructorTables();                       \
    static void destroy##argNames##ConstructorTables();
```

To:

```cpp
// AFTER:
#define declareRunTimeSelectionTable(autoPtr,baseType,argNames,argList,parList) \
    typedef HashTable<...> argNames##ConstructorTable;                          \
    static argNames##ConstructorTable*& argNames##ConstructorTablePtr_();       \
    static void construct##argNames##ConstructorTables();                       \
    static void destroy##argNames##ConstructorTables();
```

### defineRunTimeSelectionTable Macro

The definition macro changes from:

```cpp
// BEFORE:
#define defineRunTimeSelectionTable(baseType,argNames)                          \
    baseType::argNames##ConstructorTable* baseType::argNames##ConstructorTablePtr_ = nullptr;
```

To:

```cpp
// AFTER:
#define defineRunTimeSelectionTable(baseType,argNames)                          \
    baseType::argNames##ConstructorTable*&                                      \
    baseType::argNames##ConstructorTablePtr_()                                  \
    {                                                                           \
        static argNames##ConstructorTable* ptr = nullptr;                       \
        return ptr;                                                             \
    }
```

### Usage Sites Update

All code that accessed `constructorTablePtr_` directly must change to call the function:

```cpp
// BEFORE:
if (!constructorTablePtr_)
{
    constructorTablePtr_ = new ConstructorTable;
}
constructorTablePtr_->insert(typeName, constructorPtr);

// AFTER:
if (!constructorTablePtr_())
{
    constructorTablePtr_() = new ConstructorTable;
}
constructorTablePtr_()->insert(typeName, constructorPtr);
```

## Files Affected

### Core Macro Files
- `src/OpenFOAM/db/runTimeSelection/construction/runTimeSelectionTables.H`
- `src/OpenFOAM/db/runTimeSelection/memberFunctions/memberFunctionSelectionTables.H`

### All *New.C Files (~200+)
Every file that uses runtime selection tables for the "New" factory pattern:
- `src/finiteVolume/fields/fvPatchFields/fvPatchField/fvPatchFieldNew.C`
- `src/TurbulenceModels/turbulenceModels/RAS/RASModel/RASModel.C`
- `src/dynamicMesh/motionSolvers/motionSolver/motionSolver.C`
- ... and many more

### Finding All Affected Files

Use this command to find files that need updating:
```bash
grep -r "ConstructorTablePtr_" src/ --include="*.C" --include="*.H" | grep -v "ConstructorTablePtr_()"
```

## Verification

After applying this fix, verify:

1. **Registration works:** When a DLL loads, its types register to the shared table
2. **Lookup works:** Code in any DLL/EXE can find types registered by other DLLs
3. **No duplicate tables:** Only one instance of each table exists

### Test Case
```cpp
// In simpleFoam (links to turbulence DLLs):
Info << "Available RAS models: "
     << RASModel::dictionaryConstructorTablePtr_()->sortedToc()
     << endl;
// Should list kEpsilon, kOmega, etc. registered by libTurbulenceModels.dll
```

## Alternative Approaches Considered

### 1. DLL Export of Static Members
```cpp
FOAM_EXPORT_DATA static ConstructorTable* tablePtr_;
```
**Problem:** Still creates separate storage in each DLL; export/import just controls visibility.

### 2. Explicit Singleton Class
```cpp
class TableRegistry {
    static TableRegistry& instance();
    HashTable<...>& getTable(const word& name);
};
```
**Problem:** Requires significant refactoring of all macro usage.

### 3. Function-Based Access (Chosen Solution)
**Advantage:** Minimal code changes - just add `()` to convert member access to function call.

## Related Changes

This fix works in conjunction with:
- `FOAM_TYPENAME_EXPORT` for TypeName static members
- `FOAM_DEFINE_STATIC_EXPORT` for static member definitions
- `/FORCE:UNRESOLVED` linker flag

Together, these changes enable OpenFOAM's full runtime type system to work correctly across Windows DLL boundaries.

## Implementation Checklist

1. [ ] Modify `runTimeSelectionTables.H` macros
2. [ ] Modify `memberFunctionSelectionTables.H` macros
3. [ ] Update all `*New.C` files that access tables directly
4. [ ] Rebuild libOpenFOAM
5. [ ] Rebuild all dependent libraries
6. [ ] Test type registration with debug output
7. [ ] Test type lookup across DLL boundaries
