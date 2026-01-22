/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

\*---------------------------------------------------------------------------*/

#include "DiagonalSolver.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type, class DType, class LUType>
Foam::DiagonalSolver<Type, DType, LUType>::DiagonalSolver
(
    const word& fieldName,
    const LduMatrix<Type, DType, LUType>& matrix,
    const dictionary& solverDict
)
:
    LduMatrix<Type, DType, LUType>::solver
    (
        fieldName,
        matrix,
        solverDict
    )
{}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type, class DType, class LUType>
void Foam::DiagonalSolver<Type, DType, LUType>::read
(
    const dictionary&
)
{}


template<class Type, class DType, class LUType>
Foam::SolverPerformance<Type>
Foam::DiagonalSolver<Type, DType, LUType>::solve
(
    Field<Type>& psi
) const
{
    const label nCells = psi.size();

    const Type* const __restrict__ sourcePtr = this->matrix_.source().cbegin();
    const DType* const __restrict__ diagPtr = this->matrix_.diag().cbegin();
    Type* __restrict__ psiPtr = psi.begin();

    foamExecutor exec;

    auto LambdaDiagSolver = [=](label celli)
    {
        if constexpr(std::is_same<DType, Type>::value)
        {
            psiPtr[celli] = cmptDivide(sourcePtr[celli], diagPtr[celli]);
        }
        else
        {
            psiPtr[celli] = sourcePtr[celli]/diagPtr[celli];
        }
    };
    exec.parallelFor(LambdaDiagSolver, nCells);

    return SolverPerformance<Type>
    (
        typeName,
        this->fieldName_,
        Zero,
        Zero,
        Zero,
        true,
        false
    );
}


// ************************************************************************* //
