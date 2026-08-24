/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2017-2019 OpenCFD Ltd.
    Copyright (C) 2026 CINECA
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
#include "fixedValue.H"
#include "ChebyshevSmoother.H"
#include "PrecisionAdaptor.H"
#include "lambdaOptMinList.H"

#include "diagonalPreconditioner.H"
#include "l1diagonalPreconditioner.H"

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

    // select rescaling values
    if (normalization_ == "userDefined")
    {
        lambdaMin_ = controlDict_.getOrDefault<scalar>("lambdaMin", 1./8);
        if( lambdaMin_ < 0 || lambdaMin_ > lambdaMax_ )
        {
            FatalErrorInFunction
                << "invalid value for lambdaMin, it must be 0 < lambdaMin < 1"
                << abort(FatalError);
        }

    }
    else if (normalization_ == "optimalVCycle")
    {
        using Chebyshev::FirstKind::lambdaOptMinList;
        if(pDegree_ > lambdaOptMinList.size())
        {
            FatalErrorInFunction
                << "poly degree greater of max supported of : " << lambdaOptMinList.size()
                << abort(FatalError);
        };
        lambdaMin_ = lambdaOptMinList[pDegree_ -1];
    }
    else
    {
        FatalErrorInFunction << "invalid value for LambdaMode" << abort(FatalError);
    }

    // select preconditioner
    if(subPreconditionerName_ == diagonalPreconditioner::typeName)
    {
        preconditioner_ = autoPtr<diagonalPreconditioner>::New(matrix, solverControls);
        // select spectral radius estimator
        spRadiusEstimator_ = eigenValueSolver::New
        (
            controlDict_.lookup("spectralRadius")
        );
    }
    else if (subPreconditionerName_ == l1diagonalPreconditioner::typeName)
    {
        preconditioner_ = autoPtr<l1diagonalPreconditioner>::New
            (
                matrix,
                interfaceBouCoeffs,
                interfaces,
                solverControls
            );
        // select spectral radius estimator
        spRadiusEstimator_ = autoPtr<fixedValue>::New(1.0);
    }
    else 
    {
        FatalErrorInFunction<< "precondtioner type: " <<
        subPreconditionerName_ << " not supported" << abort(FatalError);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::ChebyshevSmoother::readControls()
{
    pDegree_ = controlDict_.getOrDefault<label>("pDegree", 1);
    normalization_ = controlDict_.getOrDefault<word>
                     (
                        "normalization",
                        "optimalVCycle"
                     );
    subPreconditionerName_ = controlDict_.getOrDefault<word>
                             (
                                "subPreconditioner",
                                l1diagonalPreconditioner::typeName
                             );
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
    const bool useLowerCSR = controlDict_.getOrDefault<bool>("useLowerCSR", false);

    const label nCells = psi.size();

    solveScalar* __restrict__ psiPtr = psi.begin();

    solveScalarField psiOld(nCells);
    solveScalar* __restrict__ psiOldPtr = psiOld.begin();
   
    solveScalarField Apsi(nCells);
    solveScalar* __restrict__ ApsiPtr = Apsi.begin();

    solveScalarField wA(nCells);
    solveScalar* __restrict__ wAPtr = wA.begin();

    const scalar lambdaMax = lambdaMax_;
    const scalar lambdaMin = lambdaMin_;

    // get spectral radius estimate
    const scalar spRadius = const_cast<eigenValueSolver&>(spRadiusEstimator_()).maxEigenvalue
    (
        matrix_,
        preconditioner_(),
        interfaceBouCoeffs_,
        interfaces_,
        cmpt
    );

    if ((log_ >= 2) || (lduMatrix::debug >= 2))
    {
        Info << "lambdaMin: " << lambdaMin << nl;
        Info << "lambdaMax: " << lambdaMax << nl;
        Info << "spRadius: " << spRadius << nl;
    }

    foamExecutor exec;

    scalar rho0 = (lambdaMax - lambdaMin) / (lambdaMax + lambdaMin);
    const scalar alpha1 = 2 * rho0 / ((lambdaMax - lambdaMin) * spRadius);

    // --- Calculate A.psi
    matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt, useLowerCSR);

    preconditioner_->precondition(wA,source - Apsi);

    // Solution update related to the first iteration 
    // of the semi-iterative chebyshev algorithm
    // If the degree variable is set to one, the Chebyshev iteration corresponds 
    // to a preconditioner with relaxation parameter 
    // according to the specified smoothing range
    if (pDegree_ > 1)
    {
        auto LambdapDeg1 = [=](label celli)
        {
            psiOldPtr[celli] = psiPtr[celli];
            psiPtr[celli] += alpha1*wAPtr[celli];
        };
        exec.parallelFor(LambdapDeg1, nCells);
    }
    else
    {
        auto LambdapDeg1 = [=](label celli)
        {   
            psiPtr[celli] += alpha1*wAPtr[celli];
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
        scalar alpha1n = 4.* rhon / ((lambdaMax - lambdaMin) * spRadius);

        // --- Calculate A.psi
        matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt, useLowerCSR);
        
        preconditioner_->precondition(wA, source - Apsi);

        // p-th iteration of the Chebyshev method 
        auto LambdapDegN = [=](label celli)
	    {
            const scalar tmp = psiPtr[celli];

            psiPtr[celli] += alpha0n * (psiPtr[celli] - psiOldPtr[celli])
                          + alpha1n*wAPtr[celli];

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
