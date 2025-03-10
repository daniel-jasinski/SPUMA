/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Class
    Foam::executor

Description
    Frontend interface to parallel operations to be performed on device
    such as parallelFor and reduction.

SourceFiles
    executor.H
    executor.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_executor_H
#define Foam_executor_H

#include "label.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
template <typename T> class executor;

/*---------------------------------------------------------------------------*\
                          Class executor Declaration
\*---------------------------------------------------------------------------*/

template <typename T>
class executor
{

public:
    executor() = default;
    
    ~executor() = default;
    
    // No copy constructor
    executor(executor<T> &other) = delete;

    template<typename F> 
    void parallelFor(F& lambda, const label& size)
    {
        (static_cast<T*>(this))->_backendFor(lambda, size);
    }

    template<typename F> 
    void serialFor(F& lambda, const label& size)
    {
        (static_cast<T*>(this))->_backendSerialFor(lambda, size);
    }

    template<typename F, typename resultT>
    void reductionSum(F& lambda, resultT* result, const label& size)
    {
        (static_cast<T*>(this))->_backendReductionSum(lambda, result, size);
    }

    template<typename F,typename Op, typename resultT>
    void reductionCompare(F& lambda,Op& op, resultT* result, const label& size)
    {
        (static_cast<T*>(this))->_backendReductionCompare(lambda, op, result, size);
    }
    
    //operator
    void operator=(const executor<T> &) = delete;
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
