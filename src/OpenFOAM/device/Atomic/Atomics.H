#ifndef Foam_Atomics_H
#define Foam_Atomics_H

#include "Atomic.H"
#include "cpuAtomic.H"

#ifdef have_cuda
    #include "cudaAtomic.cuh"
#elif have_hip
    #include "hipAtomic.hpp"
#endif

namespace Foam
{

//- choose corresponding executor Atomic
#ifdef have_cuda
    typedef cudaAtomic foamAtomic;
#elif have_hip
    typedef hipAtomic foamAtomic;
#else
    typedef cpuAtomic foamAtomic;
#endif

} // namespace Foam


#endif
