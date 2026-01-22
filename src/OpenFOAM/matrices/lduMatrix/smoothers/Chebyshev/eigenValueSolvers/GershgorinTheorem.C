/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 CINECA
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

#include <time.h>
#include <stdlib.h>
#include "GershgorinTheorem.H"
#include "diagonalPreconditioner.H"
#include "l1diagonalPreconditioner.H"
#include "addToRunTimeSelectionTable.H"
#include "PrecisionAdaptor.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(GershgorinTheorem, 0);
    addToRunTimeSelectionTable(eigenValueSolver, GershgorinTheorem, word);
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::scalar Foam::GershgorinTheorem::maxEigenvalue
(
    const lduMatrix& matrix_,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt
)
{    
    const scalar* const __restrict__ lowerPtr =
        matrix_.lower().cbegin();

    const scalar* const __restrict__ upperPtr =
        matrix_.upper().cbegin();

    const label* const __restrict__ lPtr =
        matrix_.lduAddr().lowerAddr().cbegin();

    const label* const __restrict__ uPtr =
        matrix_.lduAddr().upperAddr().cbegin();

    scalar nFaces = matrix_.upper().size();

    scalarField lambda(matrix_.diag());
    scalar* __restrict__ lambdaPtr = lambda.begin();

    foamExecutor exec;

    auto Lambda = [=](label facei)
    {
        foamAtomic::AtomicAdd
        (
            lambdaPtr[lPtr[facei]], 
            mag(upperPtr[facei])
        );

        foamAtomic::AtomicAdd
        (
            lambdaPtr[uPtr[facei]],
            mag(lowerPtr[facei])
        );
    };
    exec.parallelFor(Lambda, nFaces);

    const scalar lmax = max(lambda);

    return lmax;
}

Foam::scalar Foam::GershgorinTheorem::maxEigenvalue
(
    const lduMatrix& matrix_,
    const lduMatrix::preconditioner& preconditioner_,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt
)
{
    const scalar* const __restrict__ lowerPtr =
        matrix_.lower().cbegin();

    const scalar* const __restrict__ upperPtr =
        matrix_.upper().cbegin();

    const label* const __restrict__ lPtr =
        matrix_.lduAddr().lowerAddr().cbegin();

    const label* const __restrict__ uPtr =
        matrix_.lduAddr().upperAddr().cbegin();

    label nFaces = matrix_.upper().size();

    const solveScalarField& rD = preconditioner_.getReciprocalD();
    const solveScalar* const __restrict__ rDPtr = 
        rD.cbegin();

    scalarField lambda
    (
        ConstPrecisionAdaptor<scalar,solveScalar>(rD)()*matrix_.diag()
    );
    scalar* __restrict__ lambdaPtr = lambda.begin();

    foamExecutor exec;

    auto Lambda = [=](label facei)
    {
        foamAtomic::AtomicAdd
        (
            lambdaPtr[lPtr[facei]], 
            mag(rDPtr[lPtr[facei]]*upperPtr[facei])
        );

        foamAtomic::AtomicAdd
        (
            lambdaPtr[uPtr[facei]],
            mag(rDPtr[uPtr[facei]]*lowerPtr[facei])
        );
    };
    exec.parallelFor(Lambda, nFaces);

    // add off proc contributions
    forAll(interfaces_, i)
    {
        auto* intf = interfaces_.get(i);
        if(intf)
        {
            const labelUList& faceCell = (intf->interface()).faceCells();
            const auto faceCellsPtr=faceCell.cbegin();
            const auto coeffsPtr = interfaceBouCoeffs_[i].cbegin();

            auto LambdaOffProc = [=](label elemI)
            {
                foamAtomic::AtomicAdd(lambdaPtr[faceCellsPtr[elemI]], mag(coeffsPtr[elemI]));
            };
            exec.parallelFor(LambdaOffProc, faceCell.size());
        }
    }

    return max(lambda);
}

// ************************************************************************* //
