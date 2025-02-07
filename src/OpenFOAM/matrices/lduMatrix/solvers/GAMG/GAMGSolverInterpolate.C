/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2015 OpenFOAM Foundation
    Copyright (C) 2017-2019 OpenCFD Ltd.
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

#include "GAMGSolver.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::GAMGSolver::interpolate
(
    solveScalarField& psi,
    solveScalarField& Apsi,
    const lduMatrix& m,
    const FieldField<Field, scalar>& interfaceBouCoeffs,
    const lduInterfaceFieldPtrsList& interfaces,
    const direction cmpt
) const
{
    solveScalar* __restrict__ psiPtr = psi.begin();

    const label* const __restrict__ uPtr = m.lduAddr().upperAddr().cbegin();
    const label* const __restrict__ lPtr = m.lduAddr().lowerAddr().cbegin();

    const scalar* const __restrict__ diagPtr = m.diag().cbegin();
    const scalar* const __restrict__ upperPtr = m.upper().cbegin();
    const scalar* const __restrict__ lowerPtr = m.lower().cbegin();

    Apsi = 0;
    solveScalar* __restrict__ ApsiPtr = Apsi.begin();

    const label startRequest = UPstream::nRequests();

    m.initMatrixInterfaces
    (
        true,
        interfaceBouCoeffs,
        interfaces,
        psi,
        Apsi,
        cmpt
    );

    foamExecutor exec;
    const label nFaces = m.upper().size();
    auto LambdaOffDiag = [=](label face)
    {
        foamAtomic::AtomicAdd(ApsiPtr[uPtr[face]], lowerPtr[face]*psiPtr[lPtr[face]]);
        foamAtomic::AtomicAdd(ApsiPtr[lPtr[face]], upperPtr[face]*psiPtr[uPtr[face]]);
    };
    exec.parallelFor(LambdaOffDiag, nFaces);

    m.updateMatrixInterfaces
    (
        true,
        interfaceBouCoeffs,
        interfaces,
        psi,
        Apsi,
        cmpt,
        startRequest
    );

    const label nCells = m.diag().size();
    auto LambdaDiag = [=](label celli)
    {
        psiPtr[celli] = -ApsiPtr[celli]/(diagPtr[celli]);
    };
    exec.parallelFor(LambdaDiag, nCells);
}


void Foam::GAMGSolver::interpolate
(
    solveScalarField& psi,
    solveScalarField& Apsi,
    const lduMatrix& m,
    const FieldField<Field, scalar>& interfaceBouCoeffs,
    const lduInterfaceFieldPtrsList& interfaces,
    const labelList& restrictAddressing,
    const solveScalarField& psiC,
    const direction cmpt
) const
{
    interpolate
    (
        psi,
        Apsi,
        m,
        interfaceBouCoeffs,
        interfaces,
        cmpt
    );

    const label nCells = m.diag().size();
    solveScalar* __restrict__ psiPtr = psi.begin();
    const scalar* const __restrict__ diagPtr = m.diag().cbegin();
    const solveScalar* const __restrict__ psiCPtr = psiC.cbegin();
    const label* const __restrict__ restrictAddressingPtr = restrictAddressing.cbegin();

    const label nCCells = psiC.size();
    solveScalarField corrC(nCCells, 0);
    solveScalar* __restrict__ corrCPtr = corrC.begin();

    solveScalarField diagC(nCCells, 0);
    solveScalar* __restrict__ diagCPtr = diagC.begin();

    foamExecutor exec;
    auto Lambda1 = [=](label celli)
    {
        foamAtomic::AtomicAdd(corrCPtr[restrictAddressingPtr[celli]], diagPtr[celli]*psiPtr[celli]);
        foamAtomic::AtomicAdd(diagCPtr[restrictAddressingPtr[celli]], diagPtr[celli]);
    };
    exec.parallelFor(Lambda1, nCells);

    auto Lambda2 = [=](label ccelli)
    {
        corrCPtr[ccelli] = psiCPtr[ccelli] - corrCPtr[ccelli]/diagCPtr[ccelli];
    };
    exec.parallelFor(Lambda2, nCCells);

    auto Lambda3 = [=](label celli)
    {
        psiPtr[celli] += corrCPtr[restrictAddressingPtr[celli]];
    };
    exec.parallelFor(Lambda3, nCells);
}


// ************************************************************************* //
