/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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

#include "aDICPreconditioner.H"
#include "tmp.H"
#include <algorithm>

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(aDICPreconditioner, 0);

    lduMatrix::preconditioner::
        addsymMatrixConstructorToTable<aDICPreconditioner>
        addaDICPreconditionerSymMatrixConstructorToTable_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::aDICPreconditioner::aDICPreconditioner
(
    const lduMatrix::solver& sol,
    const dictionary&
)
:
    lduMatrix::preconditioner(sol),
    rD_(sol.matrix().diag().size())
{
    const scalarField& diag = sol.matrix().diag();
    rD_ = diag;

    calcReciprocalD(rD_, sol.matrix());
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::aDICPreconditioner::calcReciprocalD
(
    solveScalarField& rD,
    const lduMatrix& matrix
)
{
    solveScalar* __restrict__ rDPtr = rD.begin(); 

    tmp<scalarField> rDTmp = tmp<scalarField>::New(rD);
    scalarField& rDtmp = rDTmp.ref();
    solveScalar* __restrict__ rDtmpPtr = rDtmp.begin();

    const label* const __restrict__ uPtr = matrix.lduAddr().upperAddr().cbegin();
    const label* const __restrict__ lPtr = matrix.lduAddr().lowerAddr().cbegin();

    const scalar* const __restrict__ upperPtr = matrix.upper().cbegin();

    label nFaces = matrix.upper().size();

    foamExecutor exec;
    auto Lambda1 = [=](label face)
    {
        foamAtomic::AtomicAdd
	(
	    rDtmpPtr[uPtr[face]], 
           -upperPtr[face]*upperPtr[face]/rDPtr[lPtr[face]]
	);
    };
    exec.parallelFor(Lambda1, nFaces);

    // Calculate the reciprocal of the preconditioned diagonal
    const label nCells = rD.size();

    auto Lambda2 = [=](label cell)
    {
        rDPtr[cell] = 1.0/rDtmpPtr[cell];
    };
    exec.parallelFor(Lambda2, nCells);
}


void Foam::aDICPreconditioner::precondition
(
    solveScalarField& wA,
    const solveScalarField& rA,
    const direction
) const
{
    solveScalar* __restrict__ wAPtr = wA.begin();
    const solveScalar* __restrict__ rAPtr = rA.cbegin();
    const solveScalar* __restrict__ rDPtr = rD_.cbegin();

    const label* const __restrict__ uPtr =
        solver_.matrix().lduAddr().upperAddr().cbegin();
    const label* const __restrict__ lPtr =
        solver_.matrix().lduAddr().lowerAddr().cbegin();

    const scalar* const __restrict__ upperPtr =
        solver_.matrix().upper().cbegin();

    const label nCells = wA.size();
    const label nFaces = solver_.matrix().upper().size();

    foamExecutor exec;
    auto Lambda1 = [=](label cell)
    {
        wAPtr[cell] = rDPtr[cell]*rAPtr[cell];
    };
    exec.parallelFor(Lambda1, nCells);

    tmp<scalarField> wATmp = tmp<scalarField>::New(wA);
    scalarField& wAtmp = wATmp.ref();
    solveScalar* __restrict__ wAtmpPtr = wAtmp.begin();

    auto Lambda2 = [=](label face)
    {
        foamAtomic::AtomicAdd
        (
            wAtmpPtr[uPtr[face]],
           -rDPtr[uPtr[face]]*upperPtr[face]*wAPtr[lPtr[face]]
        );
    };
    exec.parallelFor(Lambda2, nFaces);

    wA = wAtmp;
 
    auto Lambda3 = [=](label face)
    {
        foamAtomic::AtomicAdd
        (
            wAtmpPtr[lPtr[face]],
           -rDPtr[lPtr[face]]*upperPtr[face]*wAPtr[uPtr[face]]
        );
    };
    exec.parallelFor(Lambda3, nFaces);

    wA = wAtmp;
}


void Foam::aDICPreconditioner::preconditionT
(
    solveScalarField& wT,
    const solveScalarField& rT,
    const direction
) const
{
    solveScalar* __restrict__ wTPtr = wT.begin();
    const solveScalar* __restrict__ rTPtr = rT.cbegin();
    const solveScalar* __restrict__ rDPtr = rD_.cbegin();

    const label* const __restrict__ uPtr =
        solver_.matrix().lduAddr().upperAddr().cbegin();
    const label* const __restrict__ lPtr =
        solver_.matrix().lduAddr().lowerAddr().cbegin();

    const scalar* const __restrict__ upperPtr =
        solver_.matrix().upper().cbegin();

    const label nCells = wT.size();
    const label nFaces = solver_.matrix().upper().size();

    foamExecutor exec;
    auto Lambda1 = [=](label cell)
    {
        wTPtr[cell] = rDPtr[cell]*rTPtr[cell];
    };
    exec.parallelFor(Lambda1, nCells);

    tmp<scalarField> wTTmp = tmp<scalarField>::New(wT);
    scalarField& wTtmp = wTTmp.ref();
    solveScalar* __restrict__ wTtmpPtr = wTtmp.begin();

    auto Lambda2 = [=](label face)
    {
        foamAtomic::AtomicAdd
        (
            wTtmpPtr[uPtr[face]],
           -rDPtr[uPtr[face]]*upperPtr[face]*wTPtr[lPtr[face]]
        );
    };
    exec.parallelFor(Lambda2, nFaces);

    wT = wTtmp;

    auto Lambda3 = [=](label face)
    {
        foamAtomic::AtomicAdd
        (
            wTtmpPtr[lPtr[face]],
           -rDPtr[lPtr[face]]*upperPtr[face]*wTPtr[uPtr[face]]
        );
    };
    exec.parallelFor(Lambda3, nFaces);

    wT = wTtmp;
}


// ************************************************************************* //
