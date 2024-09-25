#ifndef Foam_memory_Executors_H
#define Foam_memory_Executors_H

#include "memoryExecutor.H"

#include "cpuMemoryExecutor.H"
#ifdef have_cuda
    #include "cudaMemoryExecutor.H"
#endif

namespace Foam
{

#ifdef have_cuda
    typedef cudaMemoryExecutor foamMemoryExecutor;
#else
    typedef cpuMemoryExecutor foamMemoryExecutor;
#endif


}; // namespace Foam



#endif
