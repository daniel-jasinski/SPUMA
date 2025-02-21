#ifndef Foam_cuda_deviceInit_H
#define Foam_cuda_deviceInit_H

#include "deviceInit.H"
#include "label.H"
#include "IPstream.H"
#include "OPstream.H"
#ifdef have_cuda
    #include <cuda.h>
    #include <cuda_runtime_api.h>
#endif

namespace Foam
{
class cudaDeviceInit
: public deviceInit<cudaDeviceInit>
{
private:
    /* data */
public:

    static void _backendInit()
    {
        if (!initDeviceFlag_)
        {
            Info << "Initializing CUDA devices..." << nl << nl;

            label nDevs;
            cudaGetDeviceCount(&nDevs);

            label devID = Pstream::myProcNo() % nDevs;
            cudaSetDevice(devID);

            initDeviceFlag_ = true;
        }
    }
};

} // namespace Foam

#endif
