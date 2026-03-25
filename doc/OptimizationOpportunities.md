# Optimization Opportunities

## GAMG on SYCL: coarse-level executor overhead

### Context

While reviewing `src/OpenFOAM/matrices/lduMatrix/solvers/GAMG/GAMGSolver.C` and the hot-path helpers under `src/OpenFOAM/matrices/lduMatrix/solvers/GAMG/`, the dominant cost did not look like one expensive arithmetic kernel. The bigger issue is that GAMG performs many small transfer and scaling operations per V-cycle:

- `GAMGAgglomeration::restrictField()` uses `AtomicAdd`-based gather kernels.
- `GAMGAgglomeration::prolongField()` launches a separate injection kernel.
- `GAMGSolver::scale()` performs `Amul`, then two separate reductions, then an update kernel.
- Optional `GAMGSolver::interpolate()` adds more atomic accumulation and temporary fields.

This pattern is especially hostile to the current SYCL executor implementation in `src/OpenFOAM/device/executor/syclExecutor.cpp`, where:

- every `parallelFor()` ends with `.wait()`
- every GPU reduction does a fresh `sycl::malloc_shared` and `sycl::free`

On coarse GAMG levels, the actual amount of arithmetic becomes small enough that launch/sync/allocation overhead can dominate.

### Proposed optimization

Introduce a small-size host fallback in the SYCL executor for GPU runs, for example:

- execute `parallelFor()` on the host when `size <= 8k`
- execute reductions on the host for similarly small sizes

Additionally, reuse a 1-element USM scratch buffer per reduction type instead of allocating/freeing it on every reduction call.

### Why this is attractive

- It directly targets GAMG coarse levels, where kernel launch overhead is worst.
- It avoids changing GAMG numerics or cycle structure.
- It should also help other small-kernel solver paths, not just GAMG.

### Why this was not merged immediately

The change was prototyped and then reverted to keep behavior unchanged until it is benchmarked properly. The threshold is hardware-dependent, and the executor is shared by more than GAMG, so it should be validated with at least:

- a representative GAMG pressure solve on SYCL/CUDA
- a non-GAMG iterative solver to confirm no regression
- CPU/OMP fallback behavior

### Additional GAMG observations

- `cacheAgglomeration` only caches the `GAMGAgglomeration` mesh object. Common segregated solve paths still construct a fresh `lduMatrix::solver`, so coarse matrices and smoothers are rebuilt per solver instance.
- `scaleCorrection` is likely expensive on GPU because it adds an `Amul` and two reductions at each level during the prolongation phase.
- `interpolateCorrection` is particularly expensive if enabled, due to extra temporary fields and atomic accumulations. Keeping it off is likely important for GPU performance unless it clearly reduces iteration count enough to compensate.
