# Agent Handoff Specification

**Date**: 2026-02-14
**Status**: Fix #27 source changes complete, full rebuild of libfiniteVolume.dll needed

---

## Required Reading Before Starting

1. **`doc/windows-sycl-cuda/Debugging-Conclusions.md`** - All 27 fixes applied so far with root causes and lessons learned
2. **`doc/windows-sycl-cuda/Current-Status-Next-Steps.md`** - Build system knowledge, file inventory, environment setup
3. **`CLAUDE.md`** - Safety rules and project overview
4. **Auto-memory** at `~/.claude/projects/C--OpenFOAM-SPUMA/memory/MEMORY.md` - Build system patterns and important notes

---

## Current Problem

simpleFoam crashes during `fvMatrix<vector>::solveSegregated()` at `pTraits<Type>::componentNames[cmpt]` (fvMatrixSolve.C line 238/243). The VectorSpace template static data members (componentNames, zero, one, max, min, rootMax, rootMin, typeName) in libOpenFOAM.dll are accessed from libfiniteVolume.dll through JMP thunks instead of proper `__imp_` (dllimport) references, causing the code to read JMP instruction bytes as data pointers/values.

### Root Cause (Fix #27)

`__declspec(dllimport)` on template class static data members is **ignored by Clang/MSVC** for implicit template instantiations. Even though `FOAM_EXPORT_DATA` was applied to all VectorSpace members in Fix #26, downstream .o files still compiled WITHOUT `__imp_` prefix on these symbols.

### Solution Applied (Fix #27)

Explicit specialization declarations for VectorSpace static data members in type-specific headers, guarded by `#if defined(_WIN32) && !defined(FOAM_BUILDING_LIB)`:

```cpp
#define FOAM_VECTORSPACE_EXTERN_DATA(Form, Cmpt, N)                           \
    template<> FOAM_EXPORT_DATA const char* const                             \
        Foam::VectorSpace<Form, Cmpt, N>::typeName;                           \
    template<> FOAM_EXPORT_DATA const char* const                             \
        Foam::VectorSpace<Form, Cmpt, N>::componentNames[];                   \
    template<> FOAM_EXPORT_DATA const Form                                    \
        Foam::VectorSpace<Form, Cmpt, N>::zero;                               \
    template<> FOAM_EXPORT_DATA const Form                                    \
        Foam::VectorSpace<Form, Cmpt, N>::one;                                \
    template<> FOAM_EXPORT_DATA const Form                                    \
        Foam::VectorSpace<Form, Cmpt, N>::max;                                \
    template<> FOAM_EXPORT_DATA const Form                                    \
        Foam::VectorSpace<Form, Cmpt, N>::min;                                \
    template<> FOAM_EXPORT_DATA const Form                                    \
        Foam::VectorSpace<Form, Cmpt, N>::rootMax;                            \
    template<> FOAM_EXPORT_DATA const Form                                    \
        Foam::VectorSpace<Form, Cmpt, N>::rootMin;
```

Applied to:
- `vector.H`: Vector<float/double> (N=3)
- `tensor.H`: Tensor<float/double> (N=9)
- `symmTensor.H`: SymmTensor<float/double> (N=6)
- `sphericalTensor.H`: SphericalTensor<float/double> (N=1)

All modified headers have been copied to `src/OpenFOAM/lnInclude/`.

### Verification

Verified with `llvm-nm fvMesh.o | grep componentNames` that after recompiling with Fix #27 headers, ALL VectorSpace static data references have `__imp_` prefix. `libfiniteVolume.dll` linked without errors.

---

## What Needs to Be Done

### Step 1: Full Rebuild of libfiniteVolume.dll (~4 hours)

All .o files in the finiteVolume build directory have been deleted (0 .o files exist). A full rebuild is needed.

Use the provided rebuild script:
```bash
cd /c/OpenFOAM/SPUMA
bash rebuild-fv-sf-fix27.sh 2>&1 | tee rebuild_fix27.log
```

The script:
1. Sets up wmake environment
2. Runs `wmake -k libso` (continues past errors)
3. If `singleCellFvMesh.o` fails (pre-existing CUDA device pass error), compiles it manually with `--acpp-targets=omp`
4. Re-runs wmake to link
5. Rebuilds simpleFoam.exe

**Known issue**: `singleCellFvMesh.C` fails CUDA device compilation (sm_86 target). The rebuild script handles this by compiling with `--acpp-targets=omp` (host-only) as a fallback.

### Step 2: Test simpleFoam

```bash
cd /c/OpenFOAM/SPUMA/tutorials/incompressible/simpleFoam/pitzDaily
PATH="/c/OpenFOAM/SPUMA/platforms/win64MsvcSyclDPInt32Opt/lib:/c/OpenFOAM/SPUMA/platforms/win64MsvcSyclDPInt32Opt/bin:/c/dev/llvm-acpp-msvc-21/bin:$PATH" \
  WM_PROJECT_DIR="/c/OpenFOAM/SPUMA" WM_PROJECT="OpenFOAM" FOAM_INST_DIR="/c/OpenFOAM" \
  ACPP_VISIBILITY_MASK=omp \
  simpleFoam.exe 1>stdout.txt 2>stderr.txt
echo "EXIT=$?"
```

### Step 3: If Crash Persists After Fix #27

If simpleFoam still crashes after the rebuild, possible causes:

1. **More VectorSpace types needed**: `labelVector` (`Vector<label>`), `Vector2D`, `SymmTensor2D`, `Tensor2D` may also need `FOAM_VECTORSPACE_EXTERN_DATA` declarations. Check crash location with:
   ```bash
   llvm-nm /c/OpenFOAM/SPUMA/build/MsvcSyclDPInt32Opt/src/finiteVolume/<path>/<file>.o | grep -v __imp_ | grep "VectorSpace"
   ```

2. **Other downstream DLLs need rebuild**: If the crash is in libturbulenceModels.dll or other DLLs (not finiteVolume), those need full rebuilds too.

3. **New cross-DLL data access**: Follow the established debugging pattern:
   - Get crash RVA from Windows Event Log
   - Compute VA = ImageBase + RVA
   - `llvm-objdump -d --start-address=VA` to find the crashing function
   - Check if the data address falls in `.text` (JMP thunk) or `.data`
   - Apply `FOAM_EXPORT_DATA` or use function accessor

### Step 4: After simpleFoam Works on CPU

1. Test with CUDA: Remove `ACPP_VISIBILITY_MASK=omp` and run with `ACPP_VISIBILITY_MASK=cuda`
2. Test with GPU memory pool: Add `-pool fixedSizeMemoryPool -poolSize 32`
3. Remove all debug traces (listed in CLAUDE.md) before committing

---

## Environment Setup

The `source etc/bashrc` only works in bash subshells. Use build scripts or:

```bash
bash -c 'cd /c/OpenFOAM/SPUMA && export ACPP_PATH="/c/dev/llvm-acpp-msvc-21" && export ACPP_TARGETS="cuda:sm_86" && export CUDA_PATH_WIN="C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.5" && export have_sycl=true && export FOAM_INST_DIR=/c/OpenFOAM && source etc/bashrc && <your command here>'
```

Or use the explicit environment setup in `rebuild-fv-sf-fix27.sh` as a template.

---

## Key Files

| File | Purpose |
|------|---------|
| `src/OpenFOAM/primitives/Vector/floats/vector.H` | Fix #27: FOAM_VECTORSPACE_EXTERN_DATA for Vector |
| `src/OpenFOAM/primitives/Tensor/floats/tensor.H` | Fix #27: FOAM_VECTORSPACE_EXTERN_DATA for Tensor |
| `src/OpenFOAM/primitives/SymmTensor/symmTensor/symmTensor.H` | Fix #27: FOAM_VECTORSPACE_EXTERN_DATA for SymmTensor |
| `src/OpenFOAM/primitives/SphericalTensor/sphericalTensor/sphericalTensor.H` | Fix #27: FOAM_VECTORSPACE_EXTERN_DATA for SphericalTensor |
| `src/OpenFOAM/primitives/VectorSpace/VectorSpace.H` | VectorSpace template class (FOAM_EXPORT_DATA on members) |
| `src/OpenFOAM/include/stdFoam.H` | FOAM_EXPORT_DATA macro definition |
| `src/finiteVolume/fvMatrices/fvMatrix/fvMatrixSolve.C` | Crash location (componentNames access) |
| `rebuild-fv-sf-fix27.sh` | Build script for full finiteVolume + simpleFoam rebuild |
| `doc/windows-sycl-cuda/Debugging-Conclusions.md` | All 27 fixes documented |

---

## Debug Traces Still Present

Remove before committing:
- `src/OpenFOAM/fields/GeometricFields/GeometricField/GeometricField.C` - various traces
- `src/OpenFOAM/include/createMemoryPool.H` - TRACE: createMemoryPool
- `src/OpenFOAM/include/createTime.H` - TRACE: createTime
- `applications/utilities/mesh/generation/blockMesh/blockMesh.C` - TRACE: blockMesh
- `applications/utilities/mesh/generation/blockMesh/findBlockMeshDict.H` - TRACE: findBlockMeshDict
- `src/mesh/blockMesh/blockMesh/blockMeshTopology.C` - TRACE:topo
- `applications/solvers/incompressible/simpleFoam/simpleFoam.C` - `#include <cstdio>`
- `applications/solvers/incompressible/simpleFoam/createFields.H` - TRACE:sf

---

## Critical Patterns to Remember

1. **Cross-DLL data access via JMP thunk = crash**: Without `__declspec(dllimport)`, data symbols are accessed through JMP thunks. Code reads instruction bytes as data → garbage → crash.
2. **`__declspec(dllimport)` ignored for template class statics**: Must use explicit specialization declarations (Fix #27 pattern).
3. **Template code = cross-DLL code**: Any code in `#ifdef NoRepository` blocks gets compiled into downstream DLLs.
4. **`typeName` → `typeName_()`**: Always use function accessor for type names in cross-DLL code.
5. **`dimless` → `dimensionSet()`**: Replace cross-DLL dimension globals with default constructors in template code.
6. **lnInclude = copies on Windows**: After modifying headers/templates, MUST copy to lnInclude.
7. **`.dep.part` files**: ALWAYS rename before rebuilding: `find build -name "*.dep.part" -exec sh -c 'mv "$1" "${1%.part}"' _ {} \;`

---

## What Success Looks Like

1. **Immediate**: `simpleFoam.exe` runs to completion on pitzDaily tutorial (CPU/OMP backend)
2. **Next**: GPU acceleration works with `-pool fixedSizeMemoryPool`
3. **Final**: All debug traces removed, clean commit ready
