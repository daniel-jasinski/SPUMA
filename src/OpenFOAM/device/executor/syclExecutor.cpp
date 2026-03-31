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

#ifndef Foam_sycl_executor_cpp
#define Foam_sycl_executor_cpp

#include "syclExecutor.H"
#include "deviceM.H"
#include "zero.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<typename F>
void Foam::syclExecutor::_backendFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    sycl::queue& q = getSyclQueue();

    q.parallel_for(sycl::range<1>(size), [=](sycl::id<1> idx)
    {
        lambda(idx[0]);
    }).wait();
}

template<typename F>
void Foam::syclExecutor::_backendSerialFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    sycl::queue& q = getSyclQueue();

    q.single_task([=]()
    {
        for (label i = 0; i < size; ++i)
        {
            lambda(i);
        }
    }).wait();
}

// Generic reduction: works for any binary op (plus, min, max, custom).
// On GPU, maps to sycl::reduction which uses hardware-optimized primitives.
// On CPU, falls back to sequential loop (OMP backend broken, Fix #28).
template <typename F, typename Op, typename resultT>
void Foam::syclExecutor::_backendReduce
(
    F& lambda,
    Op op,
    resultT identity,
    resultT* const __restrict__ result,
    const label& size
)
{
    if (size <= 0) return;

    sycl::queue& q = getSyclQueue();

    if (q.get_device().is_gpu())
    {
        // GPU: sycl::reduction with identity and binary op.
        // AdaptiveCpp maps known ops (plus, min, max) to hardware-optimized
        // primitives (warp shuffles + shared memory on CUDA).
        // Custom ops get a generic tree reduction.
        resultT reduced = identity;

        {
            sycl::buffer<resultT, 1> buf(&reduced, sycl::range<1>(1));

            q.submit([&](sycl::handler& h)
            {
                auto red = sycl::reduction(buf, h, identity, op);

                h.parallel_for
                (
                    sycl::range<1>(size),
                    red,
                    [=](sycl::id<1> idx, auto& reducer)
                    {
                        reducer.combine(lambda(idx[0]));
                    }
                );
            }).wait();
        }

        *result = op(*result, reduced);
    }
    else
    {
        // CPU fallback: sycl::reduction returns 0 on AdaptiveCpp OMP
        // backend (Fix #28). Use sequential loop for correctness.
        resultT local = identity;

        for (label i = 0; i < size; ++i)
        {
            local = op(local, lambda(i));
        }

        *result = op(*result, local);
    }
}

template <typename F, typename resultT>
void Foam::syclExecutor::_backendReductionSum
(
    F& lambda,
    resultT* const __restrict__ result,
    const label& size
)
{
    _backendReduce(lambda, sycl::plus<resultT>(), resultT(Foam::Zero), result, size);
}

template <typename F, typename Op, typename resultT>
void Foam::syclExecutor::_backendReductionCompare
(
    F& lambda,
    Op& op,
    resultT* const __restrict__ result,
    const label& size
)
{
    _backendReduce(lambda, op, *result, result, size);
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
