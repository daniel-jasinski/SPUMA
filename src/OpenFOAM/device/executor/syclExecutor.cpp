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

#include <cstring>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Detail
{

//- Bitwise snapshot of a kernel functor for submission to AdaptiveCpp.
//  The CUDA and HIP executors pass the functor to a __global__ kernel by
//  value: it is copied on the host and never copied or destroyed on the
//  device. AdaptiveCpp's SSCP kernel entry instead copies the kernel object
//  inside device code, running the copy constructor and destructor of every
//  capture there. A capture that owns heap memory, such as the specie name
//  (a word) inside the pureMixture captured by the device thermos, then
//  calls std::string functions that the CUDA JIT cannot resolve, and the
//  whole kernel fails to load. The snapshot gives SYCL kernels the CUDA/HIP
//  semantics: F is copied bitwise and its special members never run on the
//  device. The submitting functor outlives the kernel (every submission
//  waits), so the snapshot never owns anything.
template<class F>
class syclKernelSnapshot
{
    union { F f_; };

public:

    explicit syclKernelSnapshot(const F& f)
    {
        std::memcpy
        (
            static_cast<void*>(&f_), static_cast<const void*>(&f), sizeof(F)
        );
    }

    syclKernelSnapshot(const syclKernelSnapshot& s)
    {
        std::memcpy
        (
            static_cast<void*>(&f_), static_cast<const void*>(&s.f_), sizeof(F)
        );
    }

    syclKernelSnapshot& operator=(const syclKernelSnapshot&) = delete;

    ~syclKernelSnapshot()
    {}

    const F& operator*() const
    {
        return f_;
    }
};

} // End namespace Detail
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<typename F>
void Foam::syclExecutor::_backendFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    sycl::queue& q = getSyclQueue();

    const Detail::syclKernelSnapshot<F> kernel(lambda);

    q.parallel_for(sycl::range<1>(size), [=](sycl::id<1> idx)
    {
        (*kernel)(idx[0]);
    }).wait();
}

template<typename F>
void Foam::syclExecutor::_backendSerialFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    sycl::queue& q = getSyclQueue();

    const Detail::syclKernelSnapshot<F> kernel(lambda);

    q.single_task([=]()
    {
        for (label i = 0; i < size; ++i)
        {
            (*kernel)(i);
        }
    }).wait();
}

// Generic reduction: works for any binary op (plus, min, max, custom).
// GPU uses sycl::reduction (USM-based). OMP uses CPU fallback (see below).
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
        // GPU: USM-based sycl::reduction (no buffers, no accessor overhead).
        // Works correctly after fixing reduction_engine.hpp value-copy bug.
        resultT* reduced = sycl::malloc_shared<resultT>(1, q);
        *reduced = identity;

        const Detail::syclKernelSnapshot<F> kernel(lambda);

        q.parallel_for
        (
            sycl::range<1>(size),
            sycl::reduction(reduced, identity, op),
            [=](sycl::id<1> idx, auto& reducer)
            {
                reducer.combine((*kernel)(idx[0]));
            }
        ).wait();

        *result = op(*result, *reduced);
        sycl::free(reduced, q);
    }
    else
    {
        // CPU fallback: sycl::reduction returns 0 on AdaptiveCpp OMP
        // backend. Use sequential loop for correctness.
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
