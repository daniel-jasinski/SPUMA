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

    if(pDegree_ > a_.size() && lambdaMode_ == 3)
    {
        FatalErrorInFunction
            <<"poly degree greater of max supported in lambdaMode: "
            << lambdaMode_ <<
            ", max poly degree supported is: "<<a_.size()
            <<abort(FatalError);
    };

    // select preconditioner
    if(preconditionerName_ == "diagonal")
    {
        preconditioner_ = autoPtr<diagonalPreconditioner>::New(matrix,solverControls);

    }
    else if (preconditionerName_ == "l1diagonal")
    {
        preconditioner_ = autoPtr<l1diagonalPreconditioner>::New(matrix,solverControls);
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
    lambdaMode_ = controlDict_.get<label>("lambdaMode");
    lambdaMax_ = controlDict_.getOrDefault<scalar>("lambdaMax", 2.0);
    lambdaMin_ = controlDict_.getOrDefault<scalar>("lambdaMin", -1);
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

    // lambda max and lambda min estimate
    scalar lambdaMax;
    scalar lambdaMin;
    // spectral radius
    scalar spRadius = 1.0;

    //TODO: clean up the Mode selection and spectral radius estimate
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
    else if (lambdaMode_ == 2)
    {
        if ((log_ >= 2) || (lduMatrix::debug >= 2))
        {
            Info << "   User-defined values for the largest and smallest eigenvalue" << nl;
        }

        // User-defined upper-bound and lower-bound
        lambdaMax = lambdaMax_;
	    lambdaMin = lambdaMin_;
    }
    else if (lambdaMode_ == 3)
    {
        if ((log_ >= 2) || (lduMatrix::debug >= 2))
        {
            Info << "Use optimal values for MG projection error" << nl;
        }
        lambdaMax = 1.0;
        lambdaMin = a_[pDegree_ -1];
        if (preconditionerName_ != "l1diagonal")
        {
            spRadius = GershgorinTheorem::maxEigenvalue
            (
                matrix_,
                interfaceBouCoeffs_,
                interfaces_,
                cmpt
            );
        }
    }
    else
    {
        FatalErrorInFunction<<"invalid value for LambdaMode"<<abort(FatalError);
    }


    // Set the minimum eigenvalue to 1/8 of the maximum eigenvalue 
    // (if not set by the user)
    if (lambdaMin_ == -1 && lambdaMode_ != 3)
    {
        lambdaMin = (1./8.) * lambdaMax;
    }

    if ((log_ >= 2) || (lduMatrix::debug >= 2))
    {
        Info << "lambdaMin: " << lambdaMin << nl;
	    Info << "lambdaMax: " << lambdaMax << nl;
    }

    foamExecutor exec;

    scalar rho0 = (lambdaMax - lambdaMin) / (lambdaMax + lambdaMin);
    scalar alpha1 = 2 * rho0 / (lambdaMax - lambdaMin);

    if (lambdaMode_ == 3) alpha1/=spRadius;

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
        scalar alpha1n = 4.* rhon / (lambdaMax - lambdaMin);

        if (lambdaMode_ == 3) alpha1n/=spRadius;

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
