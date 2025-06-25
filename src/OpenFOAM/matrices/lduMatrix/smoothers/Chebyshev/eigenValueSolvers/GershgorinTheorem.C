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

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::scalar Foam::GershgorinTheorem::maxEigenvalue
(
    const lduMatrix& matrix_,
    const FieldField<Field, scalar>& interfaceBouCoeffs_,
    const lduInterfaceFieldPtrsList& interfaces_,
    const direction cmpt
)
{
    const scalar* const __restrict__ DPtr = matrix_.diag().cbegin();
    
    const scalar* const __restrict__ lowerPtr =
        matrix_.lower().cbegin();

    const scalar* const __restrict__ upperPtr =
        matrix_.upper().cbegin();

    const label* const __restrict__ lPtr =
        matrix_.lduAddr().lowerAddr().cbegin();

    const label* const __restrict__ uPtr =
        matrix_.lduAddr().upperAddr().cbegin();

    scalar nCells = matrix_.diag().size();
    scalar nFaces = matrix_.upper().size();

    scalarField lambda(nCells, 1.0);
    scalar* __restrict__ lambdaPtr = lambda.begin();

    scalarField rD(nCells);
    scalar* __restrict__ rDPtr = rD.begin();
   
    foamExecutor exec;

    auto LambdarD = [=](label celli)
    {
        rDPtr[celli] = 1.0 / DPtr[celli];
    };
    exec.parallelFor(LambdarD, nCells);

    auto LambdaL = [=](label facei)
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
    exec.parallelFor(LambdaL, nFaces);

    scalar lmax = 1. + max(lambda);

    return lmax;
}

// ************************************************************************* //
