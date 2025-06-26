/*---------------------------------------------------------------------------*\
  *      .  *_______ * ______ .  __ *  __ * ___ .___    .  ___ .   *  .     *
    *  .    /       | |   _  \  |  |  |  | |   \/   | *   /   \ *   .    *   .
 *    .  * .\   (---*.|  |_)  |.|  |  |  |*|  \  /  |. * /  *  \  .  *     *
 =^^=^^==^^^=\   \^=^=|   ___/=^|  |^=|  |=|  |\/|  |^^=/  /=\  \^=^=^^===^^^=
 0  o  O  o---)   \ 0 |  |   0  |  o--o  |o|  |  |  | o/  _____  \ 0   o  O
     0    |_______/   |__| o   o \______/  |__| 0|__| /__/  o  \__\   o
  O   o  o        0  o      0   O        o    o       O  o     0   o    0  o
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of SPUMA.

    SPUMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SPUMA is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with SPUMA.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#include <cuda_runtime_api.h>
#include "cudaMemoryExecutor.cuh"
#include "memoryKernels.H"
#include "error.H"
#include "cudaError.cuh"

// * * * * * * * * * * * * Public Member Functions * * * * * * * * * * * * * //

void* Foam::cudaMemoryExecutor::_backendAlloc(uint64_t size)
{

    void* ptr;
    label err = CHECK_CUDA_ERROR(cudaMallocManaged((void**)&ptr, size));

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

void Foam::cudaMemoryExecutor::_backendMemCopy
(
   void* dst,
   const void* src,
   uint64_t size,
   memCopyKind kind
)
{
    label err = 0;
    if (kind == memCopyKind::memCopyHostToDevice)
        err = CHECK_CUDA_ERROR(cudaMemcpy(dst, src, (size_t) size, cudaMemcpyHostToDevice));
    else if (kind == memCopyKind::memCopyDeviceToHost)
        err = CHECK_CUDA_ERROR(cudaMemcpy(dst, src, (size_t) size, cudaMemcpyDeviceToHost));
    else if (kind == memCopyKind::memCopyDeviceToDevice)
        err = CHECK_CUDA_ERROR(cudaMemcpy(dst, src, (size_t) size, cudaMemcpyDeviceToDevice));
    else if (kind == memCopyKind::memCopyDefault)
        err = CHECK_CUDA_ERROR(cudaMemcpy(dst, src, (size_t) size, cudaMemcpyDefault));
    else
        FatalErrorInFunction << "ERROR: memCopyKind not found" << abort(FatalError);

    if (err != 0)
        FatalErrorInFunction << "ERROR: cudaMemcpy returned " << err << abort(FatalError);
}

void Foam::cudaMemoryExecutor::_backendMemSet
(
    void* ptr,
    const size_t sizeInBytes,
    const void* value,
    size_t sizeOfValue
)
{
    label err = CHECK_CUDA_ERROR
    (
        cudaMemcpyAsync
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
    Foam::device::memSetKernel<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes - sizeOfValue,
        (int) sizeOfValue,
        (char*)ptr + sizeOfValue,
        (const char*) ptr
    );
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
}

void Foam::cudaMemoryExecutor::_backendMemSetScalarOne
(
    void* ptr,
    const size_t sizeInBytes
)
{
    const int numBlocks = SET_NUM_BLOCKS(sizeInBytes);
    Foam::device::memSetOneKernel<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
    (
        sizeInBytes/sizeof(scalar),
        (scalar*)ptr
    );
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
}

void Foam::cudaMemoryExecutor::_backendMemSet
(
    void* ptr,
    const size_t sizeInBytes,
    const int value
)
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

// ************************************************************************* //
