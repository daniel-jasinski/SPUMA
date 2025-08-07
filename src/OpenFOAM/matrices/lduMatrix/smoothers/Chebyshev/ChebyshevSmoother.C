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
#include "fixedEigenValue.H"
#include "ChebyshevSmoother.H"
#include "PrecisionAdaptor.H"

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

    scalarList ChebyshevSmoother::a_ = {
        0.3333333264963328,
        0.1805358974013031,
        0.1159278474090664,
        0.0820780023852334,
        0.0618496081203676,
        0.0486605907936455,
        0.0395132940691538,
        0.0328701015026835,
        0.0278702889449934,
        0.0239987636410016,
        0.0209304530155615,
        0.0184513099814704,
        0.0164152156249015,
        0.0147195630514484,
        0.0132900883942254,
        0.0120723317797092,
        0.0110250964662371,
        0.0101170167811085,
        0.0093237783639166,
        0.0086261852644572
    };

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
                <<"invalid value for lambdaMin, it must be 0<lambdaMin<1"
                <<abort(FatalError);
        }

    }
    else if (normalization_ == "optimalVCycle")
    {
        if(pDegree_ > a_.size())
        {
            FatalErrorInFunction
                <<"poly degree greater of max supported of : "<<a_.size()
                <<abort(FatalError);
        };
        lambdaMin_ = a_[pDegree_ -1];
    }
    else
    {
        FatalErrorInFunction<<"invalid value for LambdaMode"<<abort(FatalError);
    }


    // select preconditioner
    if(preconditionerName_ == "diagonal")
    {
        preconditioner_ = autoPtr<diagonalPreconditioner>::New(matrix,solverControls);
        // select spectral radius estimator
        spRadiusEstimator_ = eigenValueSolver::New
        (
            controlDict_.getOrDefault<word>("spectralRadius", "Gershgorin")
        );

    }
    else if (preconditionerName_ == "l1diagonal")
    {
        preconditioner_ = autoPtr<l1diagonalPreconditioner>::New(matrix,solverControls);
        // select spectral radius estimator
        spRadiusEstimator_ = autoPtr<fixedEigenValue>::New(1.0);
    }
    else 
    {
        FatalErrorInFunction<<"precondtioner type: " <<
            preconditionerName_ << " not supported"<<abort(FatalError);
    }

}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

void Foam::ChebyshevSmoother::readControls()
{
    pDegree_ = controlDict_.getOrDefault<label>("pDegree", 1);
    normalization_ = controlDict_.get<word>("normalization");
    preconditionerName_ = controlDict_.get<word>("preconditionerName");
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
   
    solveScalarField Apsi(nCells);
    solveScalar* __restrict__ ApsiPtr = Apsi.begin();

    solveScalarField wA(nCells);
    solveScalar* __restrict__ wAPtr = wA.begin();


    const scalar lambdaMax = lambdaMax_;
    const scalar lambdaMin = lambdaMin_;

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
    const scalar alpha1 = 2 * rho0 / (lambdaMax - lambdaMin) /spRadius;

    // --- Calculate A.psi
    matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt);

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
        scalar alpha1n = 4.* rhon / (lambdaMax - lambdaMin)/spRadius;

        // --- Calculate A.psi
        matrix_.Amul(Apsi, psi, interfaceBouCoeffs_, interfaces_, cmpt);
        
        preconditioner_->precondition(wA,source - Apsi);

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
