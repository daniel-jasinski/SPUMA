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

#ifndef Foam_sycl_memory_executor_cpp
#define Foam_sycl_memory_executor_cpp

#include "syclMemoryExecutor.H"
#include "error.H"
#include "scalar.H"

// * * * * * * * * * * * * Public Member Functions * * * * * * * * * * * * * //

inline void* Foam::syclMemoryExecutor::_backendAlloc(uint64_t size)
{
    sycl::queue& q = getSyclQueue();

    void* ptr = sycl::malloc_shared(size, q);

    if (ptr == nullptr)
    {
        FatalErrorInFunction
            << "ERROR: sycl::malloc_shared failed for size " << size
            << abort(FatalError);
    }

    return ptr;
}

inline void Foam::syclMemoryExecutor::_backendClear(void* ptr)
{
    // Skip the free once the queue has been shut down (static-duration
    // objects destroyed after main returns): the USM allocation is
    // released with its context, and freeing into a new queue would be
    // undefined behaviour.
    if (ptr && !syclQueueShutdown())
    {
        sycl::queue& q = getSyclQueue();
        sycl::free(ptr, q);
    }
}

inline void Foam::syclMemoryExecutor::_backendMemCopy
(
    void* dst,
    const void* src,
    uint64_t size,
    memCopyKind kind
)
{
    // No-op after device shutdown (static-duration objects destroyed
    // after main returns): the queue and its allocations are gone.
    // Before first use the queue is still created lazily.
    if (syclQueueShutdown())
    {
        return;
    }

    sycl::queue& q = getSyclQueue();

    // SYCL USM handles all directions transparently
    q.memcpy(dst, src, static_cast<size_t>(size)).wait();
}

inline void Foam::syclMemoryExecutor::_backendMemSet
(
    void* ptr,
    const size_t sizeInBytes,
    const void* value,
    size_t sizeOfValue
)
{
    // No-op after device shutdown (static-duration objects destroyed
    // after main returns): the queue and its allocations are gone.
    // Before first use the queue is still created lazily.
    if (syclQueueShutdown())
    {
        return;
    }

    sycl::queue& q = getSyclQueue();

    // Copy first element
    q.memcpy(ptr, value, sizeOfValue).wait();

    // Replicate pattern to remaining bytes
    if (sizeInBytes > sizeOfValue)
    {
        const size_t numElements = (sizeInBytes - sizeOfValue) / sizeOfValue;
        char* destPtr = static_cast<char*>(ptr) + sizeOfValue;
        const char* srcPtr = static_cast<const char*>(ptr);

        q.parallel_for(sycl::range<1>(numElements), [=](sycl::id<1> idx)
        {
            const size_t offset = idx[0] * sizeOfValue;
            for (size_t j = 0; j < sizeOfValue; ++j)
            {
                destPtr[offset + j] = srcPtr[j];
            }
        }).wait();
    }
}

inline void Foam::syclMemoryExecutor::_backendMemSetScalarOne
(
    void* ptr,
    const size_t sizeInBytes
)
{
    // No-op after device shutdown (static-duration objects destroyed
    // after main returns): the queue and its allocations are gone.
    // Before first use the queue is still created lazily.
    if (syclQueueShutdown())
    {
        return;
    }

    sycl::queue& q = getSyclQueue();

    const size_t numScalars = sizeInBytes / sizeof(scalar);
    scalar* scalarPtr = static_cast<scalar*>(ptr);

    q.parallel_for(sycl::range<1>(numScalars), [=](sycl::id<1> idx)
    {
        scalarPtr[idx[0]] = scalar(1);
    }).wait();
}

inline void Foam::syclMemoryExecutor::_backendMemSet
(
    void* ptr,
    const size_t sizeInBytes,
    const int value
)
{
    // No-op after device shutdown (static-duration objects destroyed
    // after main returns): the queue and its allocations are gone.
    // Before first use the queue is still created lazily.
    if (syclQueueShutdown())
    {
        return;
    }

    sycl::queue& q = getSyclQueue();

    q.memset(ptr, value, sizeInBytes).wait();
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
