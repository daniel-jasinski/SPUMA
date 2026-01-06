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

Class
    Foam::syclExecutor

Description
    SYCL executor backend using AdaptiveCpp.

SourceFiles
    syclExecutor.H
    syclExecutor.cpp

\*---------------------------------------------------------------------------*/

#ifndef Foam_sycl_executor_H
#define Foam_sycl_executor_H

#include "executor.H"
#include <sycl/sycl.hpp>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration
class syclExecutor;

// Global SYCL queue accessor
sycl::queue& getSyclQueue();

/*---------------------------------------------------------------------------*\
                          Class syclExecutor Declaration
\*---------------------------------------------------------------------------*/

class syclExecutor
:
    public executor<syclExecutor>
{

public:

    template<typename F>
    void _backendFor(F& lambda, const label& size);

    template<typename F>
    void _backendSerialFor(F& lambda, const label& size);

    template<typename F, typename resultT>
    void _backendReductionSum
    (
        F& lambda, resultT* const __restrict__ result,
        const label& size
    );

    template<typename F,typename Op, typename resultT>
    void _backendReductionCompare
    (
        F& lambda, Op& op,
        resultT* const __restrict__ result,
        const label& size
    );
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "syclExecutor.cpp"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
