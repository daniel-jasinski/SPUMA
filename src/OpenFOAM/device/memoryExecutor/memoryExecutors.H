#ifndef Foam_memory_Executors_H
#define Foam_memory_Executors_H

#include "memoryExecutor.H"

#include "cpuMemoryExecutor.H"
#ifdef have_cuda
    #include "cudaMemoryExecutor.cuh"
#endif
#ifdef have_hip
    #include "hipMemoryExecutor.hpp"
#endif

namespace Foam
{

#ifdef have_cuda
    typedef cudaMemoryExecutor foamMemoryExecutor;
#elif have_hip
    typedef hipMemoryExecutor foamMemoryExecutor;
#else
    typedef cpuMemoryExecutor foamMemoryExecutor;
#endif


}; // namespace Foam



#endif
