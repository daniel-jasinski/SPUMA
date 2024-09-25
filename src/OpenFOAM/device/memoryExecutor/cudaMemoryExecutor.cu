//#include <cuda.h>
#include <cuda_runtime_api.h>
#include "cudaMemoryExecutor.H"
#include "memoryKernels.H"
#include "error.H"
#include "cudaError.H"



// * * * * * * * * * * * * Public Member Functions * * * * * * * * * * * * * //

void* Foam::cudaMemoryExecutor::_backendAlloc(uint64_t size)
{

    void* ptr;
    #ifdef have_managed
        label err = CHECK_CUDA_ERROR(cudaMallocManaged((void**)&ptr, size));
    #else
        label err = CHECK_CUDA_ERROR(cudaMalloc((void**)&ptr, size));
    #endif

    if (err != 0)
    {
        FatalErrorInFunction << "ERROR: cudaMallocManaged returned " << err << abort(FatalError);
    }
    return ptr;
}

void Foam::cudaMemoryExecutor::_backendClear(void* ptr)
{
    if (ptr)
    {
        label err = CHECK_CUDA_ERROR(cudaFree(ptr));
        if (err != 0)
        {
            FatalErrorInFunction << "ERROR: cudaFree returned " << err << abort(FatalError);
        }
    }
}

void Foam::cudaMemoryExecutor::_backendMemCopy(void* dst, const void* src, uint64_t size, memCopyKind kind)
{
    label err = 0;
    if (kind == memCopyKind::memCopyHostToDevice)
        err = CHECK_CUDA_ERROR(cudaMemcpy(dst, src, (size_t) size, cudaMemcpyHostToDevice));
    else if (kind == memCopyKind::memCopyDeviceToHost)
        err = CHECK_CUDA_ERROR(cudaMemcpy(dst, src, (size_t) size, cudaMemcpyDeviceToHost));
    else if (kind == memCopyKind::memCopyDeviceToDevice)
        err = CHECK_CUDA_ERROR(cudaMemcpy(dst, src, (size_t) size, cudaMemcpyDeviceToDevice));
    else
        FatalErrorInFunction << "ERROR: memCopyKind not found" << abort(FatalError);

    if (err != 0)
        FatalErrorInFunction << "ERROR: cudaMemcpy returned " << err << abort(FatalError);
}

void Foam::cudaMemoryExecutor::_backendMemSet(void* ptr, const size_t sizeInBytes, const void* value, size_t sizeOfValue)
{
    label err = CHECK_CUDA_ERROR
    (
        cudaMemcpy
        (
            ptr,
            value,
            sizeOfValue,
            cudaMemcpyHostToDevice
        )
    );
    if (err != 0)
        FatalErrorInFunction << "ERROR: cudaMemcpy returned " << err << abort(FatalError);

    int numBlocks = SET_NUM_BLOCKS(sizeInBytes - sizeOfValue);
    numBlocks = numBlocks == 0 ? 1 : numBlocks;
    Foam::cuda::memSetKernel<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes - sizeOfValue,
        (int) sizeOfValue,
        (char*)ptr + sizeOfValue,
        (const char*) ptr
    );
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
}

void Foam::cudaMemoryExecutor::_backendMemSetScalarOne(void* ptr, const size_t sizeInBytes)
{
    const int numBlocks = SET_NUM_BLOCKS(sizeInBytes);
    Foam::cuda::memSetOneKernel<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes/sizeof(scalar),
        (scalar*)ptr
    );
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
}

void Foam::cudaMemoryExecutor::_backendMemSet(void* ptr, const size_t sizeInBytes, const int value)
{
    label err = CHECK_CUDA_ERROR
    (
        cudaMemset
        (
            ptr,
            value,
            sizeInBytes
        )
    );

    if (err != 0)
        FatalErrorInFunction << "ERROR: cudaMemcpy returned " << err << abort(FatalError);

}


