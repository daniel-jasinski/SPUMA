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

Struct
    Foam::cudaAtomic

Description
    CUDA atomic backend

SourceFiles
    cudaAtomic.cuh

\*---------------------------------------------------------------------------*/

#ifndef Foam_cuda_Atomic_H
#define Foam_cuda_Atomic_H

#include "Atomic.H"
#include "cudaDeviceUtils.cuh"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           struct cudaAtomic Declaration
\*---------------------------------------------------------------------------*/

struct cudaAtomic
: Atomic<cudaAtomic>
{
    template<class T>
    struct atomicPlusEqOp
    {
        FOAM_DEVICE void operator()(T& x, const T& y) const
        {
#ifdef __CUDA_ARCH__
            atomicAdd(&x,y);
#endif
        }
    };

    FOAM_DEVICE static void _backendAtomicAdd(solveScalar& x, const solveScalar& y)
    {
#ifdef __CUDA_ARCH__
        atomicAdd(&x,y);
#endif
    }

    FOAM_DEVICE static void _backendAtomicAdd(label& x, const label& y)
    {
#ifdef __CUDA_ARCH__
        atomicAdd(&x,y);
#endif
    }

    FOAM_DEVICE static void  _backendAtomicMax(solveScalar& x, const solveScalar& y)
    {
#ifdef __CUDA_ARCH__
        atomicMax(&x,y);
#endif
    }

    FOAM_DEVICE static void  _backendAtomicMax(label& x, const label& y)
    {
#ifdef __CUDA_ARCH__
        atomicMax(&x,y);
#endif
    }

    template<class T>
    struct atomicMaxEqOp
    {
        FOAM_DEVICE void operator()(T& x, const T& y) const
        {
            _backendAtomicMax(x,y);
        }
    };

    FOAM_DEVICE static void  _backendAtomicMin(solveScalar& x, const solveScalar& y)
    {
#ifdef __CUDA_ARCH__
        atomicMin(&x,y);
#endif
    }

    FOAM_DEVICE static void  _backendAtomicMin(label& x, const label& y)
    {
#ifdef __CUDA_ARCH__
        atomicMin(&x,y);
#endif
    }

    template<class T>
    struct atomicMinEqOp
    {
        FOAM_DEVICE void operator()(T& x, const T& y) const
        {
            _backendAtomicMin(x,y);
        }
    };

    template<class Form, class Cmpt, direction Ncmpts>
    FOAM_DEVICE static void _backendAtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts,0>::eqOp(vs1, vs2, atomicPlusEqOp<Cmpt>());
    }

    template<class Form, class Cmpt, direction Ncmpts>
    FOAM_DEVICE static void _backendAtomicMax
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts,0>::eqOp(vs1, vs2, atomicMaxEqOp<Cmpt>());
    }

    template<class Form, class Cmpt, direction Ncmpts>
    FOAM_DEVICE static void _backendAtomicMin
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts,0>::eqOp(vs1, vs2, atomicMinEqOp<Cmpt>());
    }

    FOAM_DEVICE static label _backendAtomicCAS
    (
        label& x,
        const label& compare,
        const label& y
    )
    {
#ifdef __CUDA_ARCH__
        return atomicCAS(&x,compare,y);
#else
        return x;
#endif
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
