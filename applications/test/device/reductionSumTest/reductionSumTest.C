/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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

Application
    reductionSumTest

Group
    test/device

Description
    Test to check the reduction operation. 
    It is hard coded for an array size of 100000.

\*---------------------------------------------------------------------------*/

#include "OSspecific.H"
#include "argList.H"
#include "Field.H"
using namespace Foam;

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Test for array reduction operation"
    );

    #include "setRootCase.H"
    #include "initDevice.H"
    #include "createMemoryPool.H"
    
    const label size = 100000;
    scalarField testArray(size);
    
    // Fill with global sequential numbering
    forAll(testArray, i)
    {
        testArray[i] = scalar(i + 1);
    }
    
    // Perform the reduction operation
    // and compare with the expected result
    // Expected sum for array of size n is n(n+1)/2
    // where n is the global size of the array.
    const scalar computedSum = sum(testArray);
    const scalar expectedSum = 0.5 * size * (size + 1);

    bool reductionSumTestPassed = false;
    const scalar tolerance = 1e-12;
    if (mag(computedSum - expectedSum) < tolerance)
    {
        reductionSumTestPassed = true;
    }
   
    // Summary of all validation tests
    Info << "\n=== reductionSumTest execution summary: ===" << nl;
    Info << "\tarray reductionSum test: " << (reductionSumTestPassed ? "PASSED" : "FAILED") << nl;

    return 0;
}

// ************************************************************************* //
