/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
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

#include "l1diagonalPreconditioner.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(l1diagonalPreconditioner, 0);

    lduMatrix::preconditioner::
        addsymMatrixConstructorToTable<l1diagonalPreconditioner>
        addl1diagonalPreconditionerSymMatrixConstructorToTable_;

    lduMatrix::preconditioner::
        addasymMatrixConstructorToTable<l1diagonalPreconditioner>
        addl1diagonalPreconditionerAsymMatrixConstructorToTable_;
}

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::l1diagonalPreconditioner::calcReciprocalD
(
    solveScalarField& rD,
    const lduMatrix& matrix
)
{
    solveScalar* __restrict__ rDPtr = rD.begin();
    const scalar* __restrict__ DPtr = matrix.diag().cbegin();

    
    const label* const __restrict__ uPtr =
        matrix.lduAddr().upperAddr().cbegin();
    const label* const __restrict__ lPtr =
        matrix.lduAddr().lowerAddr().cbegin();
    
    const solveScalar* const __restrict__ upperPtr =
        matrix.upper().cbegin();
    const solveScalar* const __restrict__ lowerPtr =
        matrix.lower().cbegin();
    
    foamExecutor exec;

    const label nCells = rD.size();


    auto LambdarD = [=](label celli)
    {
        rDPtr[celli] = mag(DPtr[celli]);
    };
    exec.parallelFor(LambdarD, nCells);
    
    const label nFaces = matrix.lduAddr().lowerAddr().size();
    auto LambdaOffDiag = [=](label face)
    {
        foamAtomic::AtomicAdd(rDPtr[uPtr[face]], mag(lowerPtr[face]));
        foamAtomic::AtomicAdd(rDPtr[lPtr[face]], mag(upperPtr[face]));
    };

    exec.parallelFor(LambdaOffDiag,nFaces);

    rD = sign(matrix.diag())/rD;
}

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::l1diagonalPreconditioner::l1diagonalPreconditioner
(
    const lduMatrix::solver& sol,
    const dictionary&
)
:
    lduMatrix::preconditioner(sol),
    rD(sol.matrix().diag().size())
{
    this->calcReciprocalD(rD,sol.matrix());
}


Foam::l1diagonalPreconditioner::l1diagonalPreconditioner
(
    const lduMatrix& matrix,
    const dictionary&
)
:
    lduMatrix::preconditioner(),
    rD(matrix.diag().size())
{
    this->calcReciprocalD(rD,matrix);
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::l1diagonalPreconditioner::precondition
(
    solveScalarField& wA,
    const solveScalarField& rA,
    const direction
) const
{
    solveScalar* __restrict__ wAPtr = wA.begin();
    const solveScalar* __restrict__ rAPtr = rA.begin();
    const solveScalar* __restrict__ rDPtr = rD.begin();

    const label nCells = wA.size();

    auto Lambda = [=](label cell)
    {
        wAPtr[cell] = rDPtr[cell]*rAPtr[cell];
    };
    foamExecutor exec;
    exec.parallelFor(Lambda, nCells);
}


// ************************************************************************* //
