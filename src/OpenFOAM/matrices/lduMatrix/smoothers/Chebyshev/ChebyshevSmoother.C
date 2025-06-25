/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2017-2019 OpenCFD Ltd.
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
#include "powerMethod.H"
#include "ChebyshevSmoother.H"
#include "PrecisionAdaptor.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(ChebyshevSmoother, 0);

    lduMatrix::smoother::addsymMatrixConstructorToTable<ChebyshevSmoother>
        addChebyshevSmootherSymMatrixConstructorToTable_;

    lduMatrix::smoother::addasymMatrixConstructorToTable<ChebyshevSmoother>
        addChebyshevSmootherAsymMatrixConstructorToTable_;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::ChebyshevSmoother::ChebyshevSmoother
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

void Foam::ChebyshevSmoother::readControls()
{
    pDegree_ = controlDict_.getOrDefault<label>("pDegree", 1);
    lambdaMode_ = controlDict_.getOrDefault<label>("lambdaMode", 2);
    lambdaMax_ = controlDict_.getOrDefault<scalar>("lambdaMax", 2.0);
    lambdaMin_ = controlDict_.getOrDefault<scalar>("lambdaMin", -1);
    log_ = controlDict_.getOrDefault<label>("log", 0);
}

void Foam::ChebyshevSmoother::smooth_
(
    const word& fieldName_,
    solveScalarField& psi,
    const lduMatrix& matrix_,
    const solveScalarField& source,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt,
    const label nSweeps
)  const
{
    const label nCells = psi.size();

    solveScalar* __restrict__ psiPtr = psi.begin();

    solveScalarField psiOld(nCells);
    solveScalar* __restrict__ psiOldPtr = psiOld.begin();

    const solveScalar* const __restrict__ bPtr = source.begin();

    scalarField rD(nCells);
    scalar* __restrict__ rDPtr = rD.begin();
    const scalar* const __restrict__ DPtr = matrix_.diag().cbegin();
   
    solveScalarField Apsi(nCells);
    solveScalar* __restrict__ ApsiPtr = Apsi.begin();

    // lambda max and lambda min estimate
    scalar lambdaMax;
    scalar lambdaMin;

    if (lambdaMode_ == 0)
    {
        if ((log_ >= 2) || (lduMatrix::debug >= 2))
        {
            Info << "   Using the powerMethod to estimate the largest eigenvalue" << nl;
        }

        // power method (experimental)
        lambdaMax = powerMethod::maxEigenvalue
	(
	    matrix_, 
	    interfaceBouCoeffs_, 
	    interfaces_, 
	    cmpt
	);
    }
    else if (lambdaMode_ == 1)
    {
        if ((log_ >= 2) || (lduMatrix::debug >= 2))
        {   
            Info << "   Using the GershgorinTheorem to estimate the largest eigenvalue" << nl;
        }

        // Gershgorin upper bound
        lambdaMax = GershgorinTheorem::maxEigenvalue
        (
            matrix_,
            interfaceBouCoeffs_,
            interfaces_,
            cmpt
        );
    }
    else
    {
        if ((log_ >= 2) || (lduMatrix::debug >= 2))
        {
            Info << "   User-defined values for the largest and smallest eigenvalue" << nl;
        }

        // User-defined upper-bound and lower-bound
        lambdaMax = lambdaMax_;
	lambdaMin = lambdaMin_;
    }

    // Set the minimum eigenvalue to 1/8 of the maximum eigenvalue 
    // (if not set by the user)
    if (lambdaMin_ == -1)
    {
        lambdaMin = (1./8.) * lambdaMax;
    }

    if ((log_ >= 2) || (lduMatrix::debug >= 2))
    {
        Info << "lambdaMin: " << lambdaMin << nl;
	Info << "lambdaMax: " << lambdaMax << nl;
    }

    foamExecutor exec;

    // We use the Jacobi preconditioner in the Chebyshev iterations
    // Generate reciprocal diagonal
    auto LambdarD = [=](label celli)
    {
        rDPtr[celli] = 1.0 / DPtr[celli];
    };
    exec.parallelFor(LambdarD, nCells);
    
    scalar rho0 = (lambdaMax - lambdaMin) / (lambdaMax + lambdaMin);
    scalar alpha1 = 2 * rho0 / (lambdaMax - lambdaMin);

    // --- Calculate A.psi
    matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt);

    // Solution update related to the first iteration 
    // of the semi-iterative chebyshev algorithm
    // If the degree variable is set to one, the Chebyshev iteration corresponds 
    // to a Jacobi preconditioner with relaxation parameter 
    // according to the specified smoothing range
    if (pDegree_ > 1)
    {
        auto LambdapDeg1 = [=](label celli)
        {
            psiOldPtr[celli] = psiPtr[celli];
            psiPtr[celli] += alpha1*rDPtr[celli]*(bPtr[celli] - ApsiPtr[celli]);
        };
        exec.parallelFor(LambdapDeg1, nCells);
    }
    else
    {
        auto LambdapDeg1 = [=](label celli)
        {   
            psiPtr[celli] += alpha1*rDPtr[celli]*(bPtr[celli] - ApsiPtr[celli]);
        };
        exec.parallelFor(LambdapDeg1, nCells);
    }

    scalar rhok = 2.* (lambdaMax + lambdaMin) / (lambdaMax - lambdaMin);
    scalar rhonminus1 = rho0;

    // Remaining iterations up to the maximum degree of the Chebyshev polynomial
    for (label p=1; p<pDegree_; ++p)
    {
        scalar rhon = 1. / (rhok - rhonminus1);
        scalar alpha0n = rhon * rhonminus1;
        scalar alpha1n = 4.* rhon / (lambdaMax - lambdaMin);

        // --- Calculate A.psi
        matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt);
        
        // p-th iteration of the Chebyshev method 
        auto LambdapDegN = [=](label celli)
	{
            const scalar tmp = psiPtr[celli];

            psiPtr[celli] += alpha0n * (psiPtr[celli] - psiOldPtr[celli])
                          + alpha1n*rDPtr[celli]*(bPtr[celli] - ApsiPtr[celli]);

            psiOldPtr[celli] = tmp;
        };
        exec.parallelFor(LambdapDegN, nCells);

    	rhonminus1 = rhon;
    }
}

void Foam::ChebyshevSmoother::smooth
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


void Foam::ChebyshevSmoother::scalarSmooth
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
