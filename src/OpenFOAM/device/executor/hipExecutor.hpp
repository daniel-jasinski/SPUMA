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
    Foam::hipExecutor

Description
    Defines backend executors for hip (AMD ROCm).

SourceFiles
    hipExecutor.hpp
    hipExecutor.hip

\*---------------------------------------------------------------------------*/
#ifndef Foam_hip_executor_H
#define Foam_hip_executor_H

#include "executor.H"

namespace Foam
{
//fwd declaration
class hipExecutor;


class hipExecutor
:
    public executor<hipExecutor>
{

public:

    //hipExecutor() = default;

    //~hipExecutor() = default;

    template<typename F>
    void _backendFor(F& lambda, const label& size);

    template<typename F>
    void _backendSerialFor(F& lambda, const label& size);

    template<typename F, typename resultT>
    void _backendReductionSum(F& lambda, resultT* const __restrict__ result, const label& size);

    template<typename F,typename Op, typename resultT>
    void _backendReductionCompare(F& lambda, Op& op, resultT* const __restrict__ result, const label& size);
};

} // namespace Foam

#ifdef NoRepository
    #include "hipExecutor.hip"
#endif

#endif
