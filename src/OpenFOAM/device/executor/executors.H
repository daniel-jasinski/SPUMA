#ifndef Foam_executors_H
#define Foam_executors_H

#include "deviceM.H"
#include "executor.H"
#include "cpuExecutor.H"

#ifdef have_cuda
    #include "cudaExecutor.cuh"
#endif

#include "Atomics.H"

#include "deviceInits.H"

namespace Foam
{

//- choose backend
#ifdef have_cuda
    typedef cudaExecutor foamExecutor;
#else
    typedef cpuExecutor foamExecutor;
#endif

}; // namespace foam

#endif
