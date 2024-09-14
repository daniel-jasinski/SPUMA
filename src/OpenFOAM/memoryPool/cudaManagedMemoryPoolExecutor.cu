/*---------------------------------------------------------------------------*\
-------------------------------------------------------------------------------
    Copyright (C) 2023-2024 CINECA
-------------------------------------------------------------------------------
License
    This file is part of zeptoFOAM.

    zeptoFOAM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    zeptoFOAM is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with zeptoFOAM.
    If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include <cuda.h>
#include <cuda_runtime_api.h>

#include "label.H"
#include "scalar.H"
#include "vector.H"
#include "tensor.H"
#include "memCopyKind.H"
#include "cudaManagedMemoryPoolExecutor.H"
#include "error.H"
#include "cudaError.H"


// * * * * * * * * * * * * * * * * CUDA Kernels  * * * * * * * * * * * * * * //

namespace Foam
{

__global__
void cudaManagedMemSet(uint64_t nElementsInBytes, int dataSize, char *target, const char *source)
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
void cudaManagedMemSetOne(uint64_t nElements, scalar *target)
{
    const scalar one = 1.0;
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    const int stride = gridDim.x * blockDim.x;
    for(int ii = idx; ii < nElements; ii += stride)
    {
        target[ii] = one;
    }
}

}


// * * * * * * * * * * * * Public Member Functions * * * * * * * * * * * * * //

void* Foam::cudaManagedMemoryPoolExecutor::alloc(uint64_t size) const
{
    void* ptr;
    label err = CHECK_CUDA_ERROR(cudaMallocManaged((void**)&ptr, size));
    if (err != 0)
    {
        FatalErrorInFunction << "ERROR: cudaMallocManaged returned " << err << abort(FatalError);
    }
    return ptr;
}

void Foam::cudaManagedMemoryPoolExecutor::clear(void* ptr) const
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

void Foam::cudaManagedMemoryPoolExecutor::memCopy(void* dst, const void* src, uint64_t size, memCopyKind kind) const
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

void Foam::cudaManagedMemoryPoolExecutor::memSet(void* ptr, const size_t sizeInBytes, const void* value, size_t sizeOfValue) const
{
    label err = CHECK_CUDA_ERROR
    (
        cudaMemcpy
        (
            ptr,
            value,
            sizeOfValue,
            cudaMemcpyDefault
        )
    );
    if (err != 0)
        FatalErrorInFunction << "ERROR: cudaMemcpy returned " << err << abort(FatalError);

    int numBlocks = SET_NUM_BLOCKS(sizeInBytes - sizeOfValue);
    numBlocks = numBlocks == 0 ? 1 : numBlocks;
    Foam::cudaManagedMemSet<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes - sizeOfValue,
        (int) sizeOfValue,
        (char*)ptr + sizeOfValue,
        (const char*) ptr
    );
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
}

void Foam::cudaManagedMemoryPoolExecutor::memSetScalarOne(void* ptr, const size_t sizeInBytes) const
{
    const int numBlocks = SET_NUM_BLOCKS(sizeInBytes);
    Foam::cudaManagedMemSetOne<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes/sizeof(scalar),
        (scalar*)ptr
    );
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
}

void Foam::cudaManagedMemoryPoolExecutor::memSet(void* ptr, const size_t sizeInBytes, const int value) const
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


