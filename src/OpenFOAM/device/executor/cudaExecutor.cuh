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
    Foam::cudaExecutor

Description
    Cuda executor backend.

SourceFiles
    cudaExecutor.cuh
    cudaExecutor.cu

\*---------------------------------------------------------------------------*/

#ifndef Foam_cuda_executor_H
#define Foam_cuda_executor_H

#include "executor.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class cudaExecutor;

/*---------------------------------------------------------------------------*\
                          Class cudaExecutor Declaration
\*---------------------------------------------------------------------------*/

class cudaExecutor
:
    public executor<cudaExecutor>
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
    #include "cudaExecutor.cu"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
