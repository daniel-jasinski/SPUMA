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
#include "powerMethod.H"
#include "FieldOps.H"
#include "Random.H"
#include "diagonalPreconditioner.H"
#include "l1diagonalPreconditioner.H"
#include "addToRunTimeSelectionTable.H"

namespace Foam
{
    defineTypeNameAndDebug(powerMethod, 0);
    addToRunTimeSelectionTable(eigenValueSolver,powerMethod,word);

}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::scalar Foam::powerMethod::maxEigenvalue
(
    const lduMatrix& matrix_,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt
)
{
    label nCells = matrix_.diag().size();

    solveScalarField lambda(nCells);
    solveScalarField Alambda(nCells, 0.0);

    // We choose a random vector as starting point
    // to decrease the chance that our vector (lambda)
    // is orthogonal to the eigenvector
    Foam::FieldOps::assign
    (
        lambda,
        lambda,
        Random::uniformGeneratorOp<scalar>(time(NULL),-1, 1)
    );

    //- evaluate normalization factor
    const scalar rlambdaNorm = 1.0/sqrt(sumSqr(lambda));

    //--- Normalize the initial vector (lambda)
    lambda *= rlambdaNorm;

    const label maxIters = 128;
    const scalar tol = 1.e-2;
    scalar lmax = 0.0;

    for (label nIter=0; nIter<maxIters; ++nIter)
    {

	    // --- Calculate (D^-1*A)*lambda
        matrix_.Amul(Alambda, lambda, interfaceBouCoeffs_, interfaces_, cmpt);

	    // --- Compute l2 norm of (D^-1*A)*lambda: |(D^-1*A)*lambda|_2
	    const scalar rAlambdaNorm = 1.0 / sqrt(sumSqr(Alambda));

	    // --- Compute lambdaMax
        lmax = sumProd(lambda,Alambda);

        const scalar err = sqrt(sumSqr(Alambda - lmax*lambda));
        //- reassign normalized approximate eigenvector
        lambda = rAlambdaNorm*Alambda;
        
        // Convergence check
        if ( err / (lmax + SMALL ) < tol)
        {
            break;
        }
    }

    if (lduMatrix::debug >= 2)
    {
        Info << "  powerMethod failed to converge" << nl;
    }

    return lmax;
}


Foam::scalar Foam::powerMethod::maxEigenvalue
(
    const lduMatrix& matrix_,
    const lduMatrix::preconditioner& preconditioner_,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt
)
{
    // --- Compute D^-1*A
    lduMatrix Pminus1Amat(matrix_); 
    Pminus1Amat *= preconditioner_.getReciprocalD();

    return powerMethod::maxEigenvalue
    (
        Pminus1Amat,
        interfaceBouCoeffs_,
        interfaces_,
        cmpt
    );

}

// ************************************************************************* //
