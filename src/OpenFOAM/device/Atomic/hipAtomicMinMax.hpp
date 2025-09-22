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
    AtomicMin/Max implementations for float and double.

SourceFiles
    hipAtomicMinMax.hpp

\*---------------------------------------------------------------------------*/

#ifndef Foam_hipAtomicMinMax_H
#define Foam_hipAtomicMinMax_H

#ifdef have_hip

#include "MemoryPool.H"
#include "hipError.hpp"
#include <hip/hip_runtime.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

__device__ __forceinline__
float hipAtomicMin(float *address, float val)
{
    float old;
    old = !signbit(val) ? __int_as_float(atomicMin((int*)address, __float_as_int(val))) :
        __uint_as_float(atomicMax((unsigned int*)address, __float_as_uint(val)));

    return old;
}

__device__ __forceinline__
double hipAtomicMin(double *address, double val)
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
float hipAtomicMax(float *address, float val)
{
    float old;
    old = !signbit(val) ? __int_as_float(atomicMax((int*)address, __float_as_int(val))) :
        __uint_as_float(atomicMin((unsigned int*)address, __float_as_uint(val)));

    return old;
}

__device__ __forceinline__
double hipAtomicMax(double *address, double val)
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

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} //end namespace Foam

// ************************************************************************* //

#endif

// ************************************************************************* //

#endif

// ************************************************************************* //

