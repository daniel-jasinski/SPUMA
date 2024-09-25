#ifndef Foam_deviceInit_H
#define Foam_deviceInit_H

#include "label.H"
#include "IPstream.H"
#include "OPstream.H"
#ifdef have_cuda
    #include <cuda.h>
    #include <cuda_runtime_api.h>
#endif

namespace Foam
{

template <typename T>
class deviceInit
{
public:

    static void Init()
    {
        T::_backendInit();
    }

};


class cpuDeviceInit
: public deviceInit<cpuDeviceInit>
{
private:
    /* data */
public:

    static void _backendInit(){};
};

#ifdef have_cuda
class cudaDeviceInit
: public deviceInit<cudaDeviceInit>
{
private:
    /* data */
public:

    static void _backendInit()
    {
        Info << "Initializing CUDA devices..." << nl << nl;

        label nDevs;
        cudaGetDeviceCount(&nDevs);

        label devID = Pstream::myProcNo() % nDevs;
        cudaSetDevice(devID);
    }
};

#endif

} // end namespace Foam

#endif
