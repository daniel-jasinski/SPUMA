#ifndef Foam_memory_Kernels_H
#define Foam_memory_Kernels_H


// * * * * * * * * * * * * * * * * CUDA Kernels  * * * * * * * * * * * * * * //

namespace Foam
{
namespace device
{


__global__
void memSetKernel(uint64_t nElementsInBytes, int dataSize, char *target, const char *source)
{
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    const int stride = gridDim.x * blockDim.x;
    for(int ii = idx; ii < nElementsInBytes; ii += stride)
    {
        int jj = ii % dataSize;
        target[ii] = source[jj];
    }
};

__global__
void memSetOneKernel(uint64_t nElements, scalar *target)
{
    const scalar one = 1.0;
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    const int stride = gridDim.x * blockDim.x;
    for(int ii = idx; ii < nElements; ii += stride)
    {
        target[ii] = one;
    }
}

} // namespace device

} // namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


#endif
