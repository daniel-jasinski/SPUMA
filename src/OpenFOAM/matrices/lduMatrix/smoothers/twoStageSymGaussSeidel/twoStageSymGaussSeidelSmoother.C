/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2017-2019 OpenCFD Ltd.
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

#include "twoStageSymGaussSeidelSmoother.H"
#include "PrecisionAdaptor.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(twoStageSymGaussSeidelSmoother, 0);

    lduMatrix::smoother::addsymMatrixConstructorToTable<twoStageSymGaussSeidelSmoother>
        addtwoStageSymGaussSeidelSmootherSymMatrixConstructorToTable_;

    lduMatrix::smoother::addasymMatrixConstructorToTable<twoStageSymGaussSeidelSmoother>
        addtwoStageSymGaussSeidelSmootherAsymMatrixConstructorToTable_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::twoStageSymGaussSeidelSmoother::twoStageSymGaussSeidelSmoother
(
    const word& fieldName,
    const lduMatrix& matrix,
    const FieldField<Field, scalar>& interfaceBouCoeffs,
    const FieldField<Field, scalar>& interfaceIntCoeffs,
    const lduInterfaceFieldPtrsList& interfaces,
    const dictionary& solverControls
)
:
    lduMatrix::smoother
    (
        fieldName,
        matrix,
        interfaceBouCoeffs,
        interfaceIntCoeffs,
        interfaces,
        solverControls
    )
{
    readControls();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::twoStageSymGaussSeidelSmoother::readControls()
{
    nInnerIter_ = controlDict_.getOrDefault<label>("nInnerIter", 1);
    omega_ = controlDict_.getOrDefault<scalar>("omega", 0.9);
}


void Foam::twoStageSymGaussSeidelSmoother::smooth_
(
    const word& fieldName_,
    solveScalarField& psi,
    const lduMatrix& matrix_,
    const solveScalarField& source,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt,
    const label nSweeps
) const
{
    solveScalar* __restrict__ psiPtr = psi.begin();
    const solveScalar* const __restrict__ bPtr = source.cbegin();

    const label nCells = psi.size();
    const label nInternalFaces = matrix_.upper().size();

    const scalar* const __restrict__ diagPtr = matrix_.diag().cbegin();

    const scalar* const __restrict__ upperPtr =
        matrix_.upper().begin();
    const scalar* const __restrict__ lowerPtr =
        matrix_.lower().begin();

    const label* const __restrict__ uPtr =
        matrix_.lduAddr().upperAddr().begin();
    const label* const __restrict__ lPtr =
        matrix_.lduAddr().lowerAddr().begin();

    const label* const __restrict__ ownStartPtr =
        matrix_.lduAddr().ownerStartAddr().begin();

    solveScalarField rDr(nCells);
    solveScalar* __restrict__ rDrPtr = rDr.begin();

    solveScalarField gOld(nCells);
    solveScalar* __restrict__ gOldPtr = gOld.begin();

    solveScalarField g(nCells);
    solveScalar* __restrict__ gPtr = g.begin();

    scalarField rD(nCells);
    scalar* __restrict__ rDPtr = rD.begin();

    const scalar omega(omega_);

    foamExecutor exec;

    // -- Calculate the inverse of the diagonal matrix (D^-1)
    auto LambdarD = [=](label celli)
    {
        rDPtr[celli] = 1. / diagPtr[celli];
    };
    exec.parallelFor(LambdarD, nCells);

    for (label sweep=0; sweep<nSweeps; sweep++)
    {
        rDr = source;

        const label startRequest = UPstream::nRequests();

        // -- exchange interface elements of current solution
        matrix_.initMatrixInterfaces
        (
            false,
            interfaceBouCoeffs_,
            interfaces_,
            psi,
            rDr,
            cmpt
        );

        matrix_.updateMatrixInterfaces
        (
            false,
            interfaceBouCoeffs_,
            interfaces_,
            psi,
            rDr,
            cmpt,
            startRequest
        );

        // -- Compute new residual vector (scaled by D^-1) for forward sweep
        // -- Initialize g with scaled residual

        auto LambdaDiag = [=](label celli)
        {
            rDrPtr[celli] -= diagPtr[celli] * psiPtr[celli];
        };
        exec.parallelFor(LambdaDiag, nCells);

        auto LambdaOffDiag = [=](label facei)
        {
            foamAtomic::AtomicAdd
            (
                rDrPtr[uPtr[facei]],
               -lowerPtr[facei] * psiPtr[lPtr[facei]]
            );

            foamAtomic::AtomicAdd
            (
                rDrPtr[lPtr[facei]],
               -upperPtr[facei] * psiPtr[uPtr[facei]]
            );
        };
        exec.parallelFor(LambdaOffDiag, nInternalFaces);

        auto LambdaScale = [=](label celli)
        {
            rDrPtr[celli] *= rDPtr[celli];
            gPtr[celli] = rDrPtr[celli];
        };
        exec.parallelFor(LambdaScale, nCells);

        // -- Perform local inner Jacobi iteration
        for (label j=0; j<this->nInnerIter_; ++j)
        {
            gOld = g;

            if (j != 0)
                g = rDr;

            // --- Multiply g by the matrix omega * D^-1 * L
            auto Lambdag = [=](label facei)
            {
                foamAtomic::AtomicAdd
                (
                    gPtr[uPtr[facei]],
                   -omega * rDPtr[uPtr[facei]] * lowerPtr[facei] * gOldPtr[lPtr[facei]]
                );
            };
            exec.parallelFor(Lambdag, nInternalFaces);
        }

        // -- Update solution vector
        psi += g;

        // --

        // -- Compute new residual vector (scaled by D^-1) for backward sweep
        auto LambdaDiag2 = [=](label celli)
        {
            rDrPtr[celli] -= rDPtr[celli] * diagPtr[celli] * gPtr[celli];
        };
        exec.parallelFor(LambdaDiag2, nCells);

        auto LambdaOffDiag2 = [=](label facei)
        {
            foamAtomic::AtomicAdd
            (
                rDrPtr[uPtr[facei]],
               -rDPtr[uPtr[facei]] * lowerPtr[facei] * gPtr[lPtr[facei]]
            );

            foamAtomic::AtomicAdd
            (
                rDrPtr[lPtr[facei]],
               -rDPtr[lPtr[facei]] * upperPtr[facei] * gPtr[uPtr[facei]]
            );
        };
        exec.parallelFor(LambdaOffDiag2, nInternalFaces);

        // -- Initialize g with scaled residual
        g = rDr;

        // -- Perform local inner Jacobi iteration
        for (label j=0; j<this->nInnerIter_; ++j)
        {
            gOld = g;

            if (j != 0)
                g = rDr;

            // --- Multiply g by the matrix omega * D^-1 * U
            auto Lambdag = [=](label facei)
            {
                foamAtomic::AtomicAdd
                (
                    gPtr[lPtr[facei]],
                   -omega * rDPtr[lPtr[facei]] * upperPtr[facei] * gOldPtr[uPtr[facei]]
                );
            };
            exec.parallelFor(Lambdag, nInternalFaces);
        }

        // -- Update solution vector
        psi += g;
    }
}


void Foam::twoStageSymGaussSeidelSmoother::smooth
(
    solveScalarField& psi,
    const scalarField& source,
    const direction cmpt,
    const label nSweeps
) const
{
    smooth_
    (
        fieldName_,
        psi,
        matrix_,
        ConstPrecisionAdaptor<solveScalar, scalar>(source),
        interfaceBouCoeffs_,
        interfaces_,
        cmpt,
        nSweeps
    );
}


void Foam::twoStageSymGaussSeidelSmoother::scalarSmooth
(
    solveScalarField& psi,
    const solveScalarField& source,
    const direction cmpt,
    const label nSweeps
) const
{
    smooth_
    (
        fieldName_,
        psi,
        matrix_,
        source,
        interfaceBouCoeffs_,
        interfaces_,
        cmpt,
        nSweeps
    );
}


// ************************************************************************* //
