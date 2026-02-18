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

template <typename F, typename resultT>
void Foam::syclExecutor::_backendReductionSum
(
    F& lambda,
    resultT* const __restrict__ result,
    const label& size
)
{
    if (size <= 0) return;

    // Host fallback: sycl::reduction with sycl::buffer returns 0
    // on the AdaptiveCpp OMP backend (built without OpenMP support).
    // Use CPU loop until proper GPU backend is available.
    resultT localSum = Foam::Zero;

    for (label i = 0; i < size; ++i)
    {
        localSum += lambda(i);
    }

    *result += localSum;
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
    if (size <= 0) return;

    // Host fallback for generic comparison reductions
    // SYCL reduction requires known identity elements
    resultT localResult = *result;

    for (label i = 0; i < size; ++i)
    {
        resultT val = lambda(i);
        localResult = op(localResult, val);
    }

    *result = localResult;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
