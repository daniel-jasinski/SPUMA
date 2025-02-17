#ifndef Foam_deviceInits_H
#define Foam_deviceInits_H

#include "cpuDeviceInit.H"
#ifdef have_cuda
    #include "cudaDeviceInit.cuh"
#elif have_hip
    #include "hipDeviceInit.hpp"
#endif

namespace Foam
{

// - choose corresponding init class
#ifdef have_cuda
    typedef cudaDeviceInit foamDeviceInit;
#elif have_hip
    typedef hipDeviceInit foamDeviceInit;
#else
    typedef cpuDeviceInit foamDeviceInit;
#endif

} // namespace Foam


#endif
