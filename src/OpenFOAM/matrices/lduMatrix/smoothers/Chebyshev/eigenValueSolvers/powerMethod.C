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

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::scalar Foam::powerMethod::maxEigenvalue
(
    const lduMatrix& matrix_,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt
)
{
    const scalar* const __restrict__ diagPtr = matrix_.diag().begin();

    label nCells = matrix_.diag().size();

    solveScalarField lambda(nCells);
    solveScalar* __restrict__ lambdaPtr = lambda.begin();

    solveScalarField Alambda(nCells, 0.0);
    solveScalar* __restrict__ AlambdaPtr = Alambda.begin();

    /* initialize random seed: */
    srand (time(NULL));

    // We choose a random vector as starting point
    // to decrease the change that our vector (lambda)
    // is orthogonal to the eigenvector
    solveScalar rlambdaNorm = 0.;
    for (label celli=0; celli<nCells; ++celli)
    {
        lambdaPtr[celli] = rand() % 10 + 0;
        rlambdaNorm += lambdaPtr[celli] * lambdaPtr[celli];
    }
    rlambdaNorm = 1.0 / sqrt(rlambdaNorm);

    //--- Normalize the initial vector (lambda)
    for (label celli=0; celli<nCells; ++celli)
    {
        lambdaPtr[celli] *= rlambdaNorm;
    }

    // --- Compute D^-1
    scalarField rD(nCells);
    scalar* __restrict__ rDPtr = rD.begin();
    for (label cell=0; cell<nCells; cell++)
    {
        rDPtr[cell] = 1.0/diagPtr[cell];
    }

    // --- Compute D^-1*A
    lduMatrix Pminus1Amat(matrix_);
    Pminus1Amat *= rD;

    solveScalar lmax = 0.;
    const label maxIters = 128;
    const solveScalar tol = 1.e-2;

    for (label nIter=0; nIter<maxIters; ++nIter)
    {
        lmax = 1.e-20;
        solveScalar rAlambdaNorm = 1.e-20;
        solveScalar AlambdaminuslmaxlambdaNorm = 0.;

        // --- Calculate (D^-1*A)*lambda
        Pminus1Amat.Amul(Alambda, lambda, interfaceBouCoeffs_, interfaces_, cmpt);

        // --- Compute l2 norm of (D^-1*A)*lambda: |(D^-1*A)*lambda|_2
        for (label celli=0; celli<nCells; ++celli)
        {
            rAlambdaNorm += AlambdaPtr[celli] * AlambdaPtr[celli];
        }
        rAlambdaNorm = 1.0 / sqrt(rAlambdaNorm);

        // --- Compute lambdaMax
        for (label celli=0; celli<nCells; ++celli)
        {
            lmax += lambdaPtr[celli] * AlambdaPtr[celli];
        }

        //---  Recompute (normalized) lambda
        for (label celli=0; celli<nCells; ++celli)
        {
            AlambdaminuslmaxlambdaNorm +=
                (AlambdaPtr[celli] - lmax * lambdaPtr[celli]) *
                (AlambdaPtr[celli] - lmax * lambdaPtr[celli]);
            lambdaPtr[celli] = rAlambdaNorm * AlambdaPtr[celli];
        }
        AlambdaminuslmaxlambdaNorm = sqrt(AlambdaminuslmaxlambdaNorm);

        // Convergence check
        if (AlambdaminuslmaxlambdaNorm / lmax < tol)
        {
            if (lduMatrix::debug >= 2)
            {
                Info << "  powerMethod converged in " << nIter << " iterations" << nl;
            }

            return lmax;
        }
    }

    if (lduMatrix::debug >= 2)
    {
        Info << "  powerMethod failed to converge" << nl;
    }

    return lmax;
}

// ************************************************************************* //
