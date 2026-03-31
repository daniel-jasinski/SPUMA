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

#include "MemoryPool.H"
#include "hipError.hpp"
#include <hip/hip_runtime.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

__device__ __forceinline__
double hipAtomicMinDouble(double *address, double val)
{
    unsigned long long ret = __double_as_longlong(*address);

    while(val < __longlong_as_double(ret))
    {
        unsigned long long old = ret;

        if
        (
            (
                ret = atomicCAS
                (
                    (
                        unsigned long long *)address,
                        old,
                        __double_as_longlong(val)
                )
            ) == old
        )
            break;
    }

    return __longlong_as_double(ret);
}

__device__ __forceinline__
double hipAtomicMaxDouble(double *address, double val)
{
    unsigned long long ret = __double_as_longlong(*address);

    while(val > __longlong_as_double(ret))
    {
        unsigned long long old = ret;

        if
        (
            (
                ret = atomicCAS
                (
                    (unsigned long long *)address,
                    old,
                    __double_as_longlong(val)
                )
            ) == old
        )
           break;
    }

    return __longlong_as_double(ret);
}

/*---------------------------------------------------------------------------*\
                          Struct Mutex Declaration
\*---------------------------------------------------------------------------*/

struct Mutex
{
    Mutex()
    {
        mutex_ = static_cast<int*>(MemoryPool::getInstance()->allocate(sizeof(int)));
        Foam::MemoryPool::getInstance()->memSet(mutex_,0,sizeof(int));
    }

    ~Mutex()
    {
        MemoryPool::getInstance()->free(mutex_);;
    }

    __host__ __device__ inline int* getMutex()
    {
        return mutex_;
    }

private:
    int* mutex_;
};


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

template <typename T,int blockSize>
__device__
void warpReduceNoVolatile( T* sdata, int tid)
{
    T tmp;
    if (blockSize >= 64) { tmp = sdata[tid + 32];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >= 32) { tmp = sdata[tid + 16];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >= 16) { tmp = sdata[tid +  8];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  8) { tmp = sdata[tid +  4];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  4) { tmp = sdata[tid +  2];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
    if (blockSize >=  2) { tmp = sdata[tid +  1];  __threadfence_block(); sdata[tid]+=tmp; __threadfence_block(); }
};


template <typename T,typename Op,int blockSize>
__device__
void warpReduceCompareNoVolatile( T* sdata, Op& op,int tid)
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

template <typename T,int blockSize>
__device__
void newWarpReduceNoVolatile( T* sdata, int tid)
{
    // hip memory model do not guarantee that reads
    // are perfomed before write: separate them

    T temp(Zero);

    if (blockSize >= 64)
    {
        temp += sdata[tid + 32]; __threadfence_block();
        sdata[tid] = temp; __threadfence_block();
    }
    if (blockSize >= 32)
    {
        temp += sdata[tid + 16]; __threadfence_block();
        sdata[tid] = temp; __threadfence_block();
    }
    if (blockSize >= 16)
    {
        temp += sdata[tid + 8]; __threadfence_block();
        sdata[tid] = temp; __threadfence_block();
    }
    if (blockSize >= 8)
    {
        temp += sdata[tid + 4]; __threadfence_block();
        sdata[tid] = temp; __threadfence_block();
    }
    if (blockSize >= 4)
    {
        temp += sdata[tid + 2]; __threadfence_block();
        sdata[tid] = temp; __threadfence_block();
    }
    if (blockSize >= 2)
    {
        temp += sdata[tid + 1]; __threadfence_block();
        sdata[tid] = temp; __threadfence_block();
    }
};

template <typename T, int blockSize>
__device__
void warpReduce(volatile T* sdata, int tid) // volataile to ensure visibility of memory operations
{
    if (blockSize >= 64) sdata[tid] += sdata[tid + 32];
    if (blockSize >= 32) sdata[tid] += sdata[tid + 16];
    if (blockSize >= 16) sdata[tid] += sdata[tid + 8];
    if (blockSize >= 8)  sdata[tid] += sdata[tid + 4];
    if (blockSize >= 4)  sdata[tid] += sdata[tid + 2];
    if (blockSize >= 2)  sdata[tid] += sdata[tid + 1];
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

