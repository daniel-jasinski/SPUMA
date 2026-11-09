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
    Foam::syclAtomic

Description
    SYCL atomic backend using sycl::atomic_ref (SYCL 2020)

SourceFiles
    syclAtomic.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_sycl_Atomic_H
#define Foam_sycl_Atomic_H

#include "Atomic.H"
#include <sycl/sycl.hpp>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         struct syclAtomic Declaration
\*---------------------------------------------------------------------------*/

struct syclAtomic
: Atomic<syclAtomic>
{
    template<class T>
    struct atomicPlusEqOp
    {
        void operator()(T& x, const T& y) const
        {
            sycl::atomic_ref<T, sycl::memory_order::relaxed,
                sycl::memory_scope::device,
                sycl::access::address_space::global_space> ref(x);
            ref += y;
        }
    };

    static void _backendAtomicAdd(solveScalar& x, const solveScalar& y)
    {
        sycl::atomic_ref<solveScalar, sycl::memory_order::relaxed,
            sycl::memory_scope::device,
            sycl::access::address_space::global_space> ref(x);
        ref += y;
    }

    static void _backendAtomicAdd(label& x, const label& y)
    {
        sycl::atomic_ref<label, sycl::memory_order::relaxed,
            sycl::memory_scope::device,
            sycl::access::address_space::global_space> ref(x);
        ref += y;
    }

    static void _backendAtomicMax(solveScalar& x, const solveScalar& y)
    {
        // SYCL atomic_ref::fetch_max for floating point
        sycl::atomic_ref<solveScalar, sycl::memory_order::relaxed,
            sycl::memory_scope::device,
            sycl::access::address_space::global_space> ref(x);
        solveScalar old = ref.load();
        while (y > old && !ref.compare_exchange_weak(old, y)) {}
    }

    static void _backendAtomicMax(label& x, const label& y)
    {
        sycl::atomic_ref<label, sycl::memory_order::relaxed,
            sycl::memory_scope::device,
            sycl::access::address_space::global_space> ref(x);
        ref.fetch_max(y);
    }

    template<class T>
    struct atomicMaxEqOp
    {
        void operator()(T& x, const T& y) const
        {
            _backendAtomicMax(x, y);
        }
    };

    static void _backendAtomicMin(solveScalar& x, const solveScalar& y)
    {
        // SYCL atomic_ref for floating point min
        sycl::atomic_ref<solveScalar, sycl::memory_order::relaxed,
            sycl::memory_scope::device,
            sycl::access::address_space::global_space> ref(x);
        solveScalar old = ref.load();
        while (y < old && !ref.compare_exchange_weak(old, y)) {}
    }

    static void _backendAtomicMin(label& x, const label& y)
    {
        sycl::atomic_ref<label, sycl::memory_order::relaxed,
            sycl::memory_scope::device,
            sycl::access::address_space::global_space> ref(x);
        ref.fetch_min(y);
    }

    template<class T>
    struct atomicMinEqOp
    {
        void operator()(T& x, const T& y) const
        {
            _backendAtomicMin(x, y);
        }
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts, 0>::eqOp(vs1, vs2, atomicPlusEqOp<Cmpt>());
    }

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMax
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts, 0>::eqOp(vs1, vs2, atomicMaxEqOp<Cmpt>());
    }

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMin
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts, 0>::eqOp(vs1, vs2, atomicMinEqOp<Cmpt>());
    }

    static label _backendAtomicCAS
    (
        label& x,
        const label& compare,
        const label& y
    )
    {
        sycl::atomic_ref<label, sycl::memory_order::relaxed,
            sycl::memory_scope::device,
            sycl::access::address_space::global_space> ref(x);
        label expected = compare;
        ref.compare_exchange_strong(expected, y);
        return expected;
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
