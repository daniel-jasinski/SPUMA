#ifndef Foam_deviceInits_H
#define Foam_deviceInits_H

#include "cpuDeviceInit.H"
#ifdef have_cuda
    #include "cudaDeviceInit.cuh"
#endif

namespace Foam
{

// - choose corresponding init class
#ifdef have_cuda
    typedef cudaDeviceInit foamDeviceInit;
#else
    typedef cpuDeviceInit foamDeviceInit;
#endif

} // namespace Foam


#endif
