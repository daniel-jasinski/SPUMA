/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2017-2019 OpenCFD Ltd.
    Copyright (C) 2026 Cineca
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

#include "twoStageGaussSeidelSmoother.H"
#include "PrecisionAdaptor.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(twoStageGaussSeidelSmoother, 0);

    lduMatrix::smoother::addsymMatrixConstructorToTable<twoStageGaussSeidelSmoother>
        addtwoStageGaussSeidelSmootherSymMatrixConstructorToTable_;

    lduMatrix::smoother::addasymMatrixConstructorToTable<twoStageGaussSeidelSmoother>
        addtwoStageGaussSeidelSmootherAsymMatrixConstructorToTable_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::twoStageGaussSeidelSmoother::twoStageGaussSeidelSmoother
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

void Foam::twoStageGaussSeidelSmoother::readControls()
{
    nInnerIter_ = controlDict_.getOrDefault<label>("nInnerIter", 1);
    omega_ = controlDict_.getOrDefault<scalar>("omega", 0.9);
}


void Foam::twoStageGaussSeidelSmoother::smooth_
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

    const bool useLowerCSR = controlDict_.getOrDefault<bool>("useLowerCSR", false);

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

    solveScalarField rDr(nCells);
    solveScalar* __restrict__ rDrPtr = rDr.begin();

    solveScalarField gOld(nCells);
    solveScalar* __restrict__ gOldPtr = gOld.begin();

    solveScalarField g(nCells);
    solveScalar* __restrict__ gPtr = g.begin();

    scalarField rD(nCells);
    scalar* __restrict__ rDPtr = rD.begin();

    const scalar omega = this->omega_;

    foamExecutor exec;

    // -- Calculate the inverse of the diagonal matrix (D^-1)
    auto LambdarD = [=](label celli)
    {
        rDPtr[celli] = 1. / diagPtr[celli];
    };
    exec.parallelFor(LambdarD, nCells);

    for (label sweep=0; sweep<nSweeps; sweep++)
    {
        // -- Compute new residual vector (scaled by D^-1)

        // --- Calculate A.psi (we use rDr as auxiliary field)
        matrix_.Amul(rDr, psi, interfaceBouCoeffs_, interfaces_, cmpt, useLowerCSR);

        // --- Calculate rDr = D^-1 * rA
        // --- Initialize g with rDr
        auto LambdarDr = [=](label celli)
        {
            rDrPtr[celli] = rDPtr[celli] * (bPtr[celli] - rDrPtr[celli]);
            gPtr[celli] = rDrPtr[celli];
        };
        exec.parallelFor(LambdarDr, nCells);

        // --- Perform local inner (nInnerIter) Jacobi iterations
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

        // --- Update solution vector
        psi += g;
    }
}


void Foam::twoStageGaussSeidelSmoother::smooth
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


void Foam::twoStageGaussSeidelSmoother::scalarSmooth
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
