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

#include "aDILUSmoother.H"
#include "aDILUPreconditioner.H"
#include "PrecisionAdaptor.H"
#include <algorithm>

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(aDILUSmoother, 0);

    lduMatrix::smoother::addasymMatrixConstructorToTable<aDILUSmoother>
        addaDILUSmootherAsymMatrixConstructorToTable_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::aDILUSmoother::aDILUSmoother
(
    const word& fieldName,
    const lduMatrix& matrix,
    const FieldField<Field, scalar>& interfaceBouCoeffs,
    const FieldField<Field, scalar>& interfaceIntCoeffs,
    const lduInterfaceFieldPtrsList& interfaces
)
:
    lduMatrix::smoother
    (
        fieldName,
        matrix,
        interfaceBouCoeffs,
        interfaceIntCoeffs,
        interfaces
    ),
    rD_(matrix_.diag().size())
{
    const scalarField& diag = matrix_.diag();
    rD_ = diag;

    aDILUPreconditioner::calcReciprocalD(rD_, matrix_);
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::aDILUSmoother::smooth
(
    solveScalarField& psi,
    const scalarField& source,
    const direction cmpt,
    const label nSweeps
) const
{
    const solveScalar* const __restrict__ rDPtr = rD_.cbegin();

    const label* const __restrict__ uPtr =
        matrix_.lduAddr().upperAddr().cbegin();
    const label* const __restrict__ lPtr =
        matrix_.lduAddr().lowerAddr().cbegin();

    const scalar* const __restrict__ upperPtr = matrix_.upper().cbegin();
    const scalar* const __restrict__ lowerPtr = matrix_.lower().cbegin();

    const label nCells = rD_.size();
    const label nFaces = matrix_.upper().size();

    // Temporary storage for the residual
    solveScalarField rA(nCells);
    solveScalar* __restrict__ rAPtr = rA.begin();

    for (label sweep=0; sweep<nSweeps; sweep++)
    {
        matrix_.residual
        (
            rA,
            psi,
            source,
            interfaceBouCoeffs_,
            interfaces_,
            cmpt
        );

        foamExecutor exec;
        auto Lambda1 = [=](label celli)
        {
            rAPtr[celli] *= rDPtr[celli];
        };
        exec.parallelFor(Lambda1, nCells);

        tmp<scalarField> rATmp = tmp<scalarField>::New(rA);
        scalarField& rAtmp = rATmp.ref();
        solveScalar* __restrict__ rAtmpPtr = rAtmp.begin();

        auto Lambda2 = [=](label face)
        {
            foamAtomic::AtomicAdd
            (
                rAtmpPtr[uPtr[face]],
               -rDPtr[uPtr[face]]*lowerPtr[face]*rAPtr[lPtr[face]]
            );
        };
        exec.parallelFor(Lambda2, nFaces);

        rA = rAtmp;

        auto Lambda3 = [=](label face)
        {
            foamAtomic::AtomicAdd
            (
                rAtmpPtr[lPtr[face]],
               -rDPtr[lPtr[face]]*upperPtr[face]*rAPtr[uPtr[face]]
            );
        };
        exec.parallelFor(Lambda3, nFaces);

        rA = rAtmp;

        psi += rA;
    }
}


void Foam::aDILUSmoother::scalarSmooth
(
    solveScalarField& psi,
    const solveScalarField& source,
    const direction cmpt,
    const label nSweeps
) const
{
    smooth
    (
        psi,
        ConstPrecisionAdaptor<scalar, solveScalar>(source),
        cmpt,
        nSweeps
    );
}


// ************************************************************************* //
