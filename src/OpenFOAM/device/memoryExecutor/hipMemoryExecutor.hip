/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/


#include <hip/hip_runtime.h>
#include "hipMemoryExecutor.hpp"
#include "deviceM.H"
#include "memoryKernels.H"
#include "error.H"
#include "hipError.hpp"

// * * * * * * * * * * * * Public Member Functions * * * * * * * * * * * * * //

__global__
void memCopyKernel
(
    const unsigned long long size,
    char* const dst,
    const char* const src
)
{
    unsigned long long idx = blockIdx.x*blockDim.x + threadIdx.x;
    unsigned long long stride = blockDim.x*gridDim.x;
    for (unsigned long long i = idx; i < size; i += stride)
    {
        dst[i] = src[i];
    }
}

void* Foam::hipMemoryExecutor::_backendAlloc(uint64_t size)
{

    void* ptr;
    label err = CHECK_HIP_ERROR(hipMallocManaged((void**)&ptr, size));
    int deviceId;
    CHECK_HIP_ERROR(hipGetDevice(&deviceId));
    CHECK_HIP_ERROR(hipMemAdvise(ptr, size, hipMemAdviseSetCoarseGrain, deviceId)); 

    if (err != 0)
    {
        FatalErrorInFunction << "ERROR: hipMallocManaged returned " << err << abort(FatalError);
    }
    return ptr;
}

void Foam::hipMemoryExecutor::_backendClear(void* ptr)
{
    if (ptr)
    {
        label err = CHECK_HIP_ERROR(hipFree(ptr));
        if (err != 0)
        {
            FatalErrorInFunction << "ERROR: hipFree returned " << err << abort(FatalError);
        }
    }
}

void Foam::hipMemoryExecutor::_backendMemCopy(void* dst, const void* src, uint64_t size, memCopyKind kind)
{
    label err = 0;
    if (kind == memCopyKind::memCopyHostToDevice)
    {
        err = CHECK_HIP_ERROR(hipMemcpy(dst, src, (size_t) size, hipMemcpyHostToDevice));
    }
    else if (kind == memCopyKind::memCopyDeviceToHost)
    {
        err = CHECK_HIP_ERROR(hipMemcpy(dst, src, (size_t) size, hipMemcpyDeviceToHost));
    }
    else if (kind == memCopyKind::memCopyDeviceToDevice)
    {
        label numblocks = SET_NUM_BLOCKS(size);
        hipLaunchKernelGGL
        (
            memCopyKernel,
            numblocks,
            NUM_THREADS_PER_BLOCK,
            0,
            0,
            size,
            reinterpret_cast<char*>(dst),
            reinterpret_cast<const char*>(src)
        );
        deviceSync();
        CHECK_LAST_HIP_ERROR();
    }
    else
        FatalErrorInFunction << "ERROR: memCopyKind not found" << abort(FatalError);

    if (err != 0)
        FatalErrorInFunction << "ERROR: hipMemcpy returned " << err << abort(FatalError);
}

void Foam::hipMemoryExecutor::_backendMemSet(void* ptr, const size_t sizeInBytes, const void* value, size_t sizeOfValue)
{
    label err = CHECK_HIP_ERROR
    (
        hipMemcpyAsync
        (
            ptr,
            value,
            sizeOfValue,
            hipMemcpyHostToDevice
        )
    );
    if (err != 0)
        FatalErrorInFunction << "ERROR: hipMemcpy returned " << err << abort(FatalError);

    int numBlocks = SET_NUM_BLOCKS(sizeInBytes - sizeOfValue);
    numBlocks = numBlocks == 0 ? 1 : numBlocks;
    Foam::device::memSetKernel<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes - sizeOfValue,
        (int) sizeOfValue,
        (char*)ptr + sizeOfValue,
        (const char*) ptr
    );
    deviceSync();
    CHECK_LAST_HIP_ERROR();
}

void Foam::hipMemoryExecutor::_backendMemSetScalarOne(void* ptr, const size_t sizeInBytes)
{
    const int numBlocks = SET_NUM_BLOCKS(sizeInBytes);
    Foam::device::memSetOneKernel<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes/sizeof(scalar),
        (scalar*)ptr
    );
    deviceSync();
    CHECK_LAST_HIP_ERROR();
}

void Foam::hipMemoryExecutor::_backendMemSet(void* ptr, const size_t sizeInBytes, const int value)
{
    label err = CHECK_HIP_ERROR
    (
        hipMemset
        (
            ptr,
            value,
            sizeInBytes
        )
    );

    if (err != 0)
        FatalErrorInFunction << "ERROR: hipMemcpy returned " << err << abort(FatalError);

}


