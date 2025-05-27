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
    Foam::cpuExecutor

Description
    CPU executor backend.

SourceFiles
    cpuExecutor.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_cpu_executor_H
#define Foam_cpu_executor_H

#include "executor.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class cpuExecutor;

/*---------------------------------------------------------------------------*\
                          Class cpuExecutor Declaration
\*---------------------------------------------------------------------------*/

class cpuExecutor
:
    public executor<cpuExecutor>
{

public:

    template<typename F>
    void _backendFor(F& lambda, const label& size)
    {
        for (label i = 0; i < size; i++)
        {
            lambda(i);
        }
    }

    template<typename F>
    void _backendSerialFor(F& lambda, const label& size)
    {
        _backendFor(lambda,size);
    }

    template<typename F, typename resultT>
    void _backendReductionSum
    (
        F& lambda,
        resultT* const __restrict__ result,
        const label& size
    )
    {
        for (label i = 0; i < size; i++)
        {
            *result += lambda(i);
        }
    }

    template<typename F,typename Op, typename resultT>
    void _backendReductionCompare
    (
        F& lambda,
        Op& op,
        resultT* const __restrict__ result,
        const label& size
    )
    {
        for (label i = 0; i < size; i++)
        {
            *result = op(lambda(i),*result);
        }
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
