/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2017-2022 OpenCFD Ltd.
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

#include "GAMGSolver.H"
#include "FixedList.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::GAMGSolver::scale
(
    solveScalarField& field,
    solveScalarField& Acf,
    const lduMatrix& A,
    const FieldField<Field, scalar>& interfaceLevelBouCoeffs,
    const lduInterfaceFieldPtrsList& interfaceLevel,
    const solveScalarField& source,
    const direction cmpt
) const
{
    const bool useLowerCSR = controlDict_.getOrDefault<bool>("useLowerCSR", false);

    A.Amul
    (
        Acf,
        field,
        interfaceLevelBouCoeffs,
        interfaceLevel,
        cmpt,
        useLowerCSR
    );

    const label nCells = field.size();
    solveScalar* __restrict__ fieldPtr = field.begin();
    const solveScalar* const __restrict__ sourcePtr = source.cbegin();
    const solveScalar* const __restrict__ AcfPtr = Acf.cbegin();

    FixedList<solveScalar, 2> scalingFactor(Zero);

    foamExecutor exec;
    auto sumOp0 = [=] (label i)
    {
        return fieldPtr[i]*sourcePtr[i];
    };

    auto sumOp1 = [=] (label i)
    {
        return fieldPtr[i]*AcfPtr[i];
    };

    exec.reductionSum(sumOp0, &scalingFactor[0], nCells);
    exec.reductionSum(sumOp1, &scalingFactor[1], nCells);

    A.mesh().reduce(scalingFactor, sumOp<solveScalar>());

    const solveScalar sf =
    (
        scalingFactor[0]
      / stabilise(scalingFactor[1], pTraits<solveScalar>::vsmall)
    );

    if (debug >= 2)
    {
        Pout<< sf << " ";
    }

    const scalarField& D = A.diag();
    const scalar* const __restrict__ DPtr = D.cbegin();

    auto Lambda = [=](label cell)
    {
        fieldPtr[cell] = sf*fieldPtr[cell] + (sourcePtr[cell] - sf*AcfPtr[cell])/DPtr[cell];
    };
    exec.parallelFor(Lambda, nCells);
}


// ************************************************************************* //
