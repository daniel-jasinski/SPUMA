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

Description
    Contains utilities for hip (AMD ROCm) device kernels.

SourceFiles
    hipDevicUtils.hpp

\*---------------------------------------------------------------------------*/

#ifndef Foam_hip_Device_Utils_H
#define Foam_hip_Device_Utils_H

#ifdef have_hip

#include "mutex.H"
#include "hipError.hpp"
#include <hip/hip_runtime.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Struct spinLock Declaration
\*---------------------------------------------------------------------------*/

struct spinLock
{
    __device__ static inline void lock(int* mutex)
    {
        while (atomicCAS(mutex, 0, 1) == 1) {};
    }
    __device__ static inline void unlock(int* mutex)
    {
        atomicExch(mutex, 0);
    }
};

namespace hip
{

template <typename T>
__device__
void warpReduceNoVolatile( T* sdata, const unsigned int tid, const unsigned int blockSize)
{
    T tmp;
    if (blockSize >= 64) { tmp = sdata[tid + 32];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >= 32) { tmp = sdata[tid + 16];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >= 16) { tmp = sdata[tid +  8];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  8) { tmp = sdata[tid +  4];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  4) { tmp = sdata[tid +  2];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  2) { tmp = sdata[tid +  1];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
};

template <typename T,typename Op>
__device__
void warpReduceCompareNoVolatile( T* sdata, Op& op, const unsigned int tid, const unsigned int blockSize)
{
    T tmp;
    if (blockSize >= 64) { tmp = op(sdata[tid],sdata[tid + 32]);__threadfence_block();
        sdata[tid]= tmp;  __threadfence_block(); }
    if (blockSize >= 32) { tmp = op(sdata[tid],sdata[tid + 16]);__threadfence_block();
        sdata[tid]= tmp;  __threadfence_block(); }
    if (blockSize >= 16) { tmp = op(sdata[tid],sdata[tid + 8] );__threadfence_block();
        sdata[tid]= tmp;   __threadfence_block(); }
    if (blockSize >= 8)  { tmp = op(sdata[tid],sdata[tid + 4] ); __threadfence_block();
        sdata[tid]= tmp;  __threadfence_block(); }
    if (blockSize >= 4)  { tmp = op(sdata[tid],sdata[tid + 2] ); __threadfence_block();
        sdata[tid]= tmp;   __threadfence_block(); }
    if (blockSize >= 2)  { tmp = op(sdata[tid],sdata[tid + 1] ); __threadfence_block();
        sdata[tid]= tmp;  __threadfence_block();  }
};

// Simple wrapper to allow the use of extern linked shared memory by
// casting the same pointer to the desired type. This prevents multiple
// definitions of the same shared pointer for different types.
template <typename T>
struct SharedMemory
{
    __device__ inline T *getPointer()
    {
        extern __shared__ __align__(8) char smem[];
        return reinterpret_cast<T*>(smem);
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace hip

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} //end namespace Foam

#endif

// ************************************************************************* //

#endif

// ************************************************************************* //

