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
    atomicAddTest 

Group
    test/device

Description
    Test for atomicAdd operation via foamAtomic.

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
        "Unit test for atomic add operation"
    );

    #include "setRootCase.H"
    #include "initDevice.H"
    #include "createMemoryPool.H"

    const label size = 400; 

    // Create a cell-based volScalarField named 'values' and initialize to 1.0
    scalarField values(size);

    // Make sure field is populated (explicit assignment to be safe)
    forAll(values, celli)
    {
        values[celli] = 1.0;
    }

    // Get raw pointer to field data
    scalar* valuesPtr = values.begin();

    // Create executor and run parallel accumulation over number of cells
    foamExecutor exec;
    auto Lambda = [=](label i)
    {
        foamAtomic::AtomicAdd(valuesPtr[0], valuesPtr[i+1]);
    };
    exec.parallelFor(Lambda, values.size()-1);

    // expected sum is number of cells * 1.0
    scalar expected = scalar(values.size()) * 1.0;

    bool atomicTestPassed = false;
    scalar tol = 1e-12;
    if (mag(valuesPtr[0] - expected) < tol)
    {
        atomicTestPassed = true;
    }

    // Summary of all validation tests
    Info << "\n=== atomicAddTest execution summary: ===" << nl;
    Info << "\tatomicAdd accumulation test: " << (atomicTestPassed ? "PASSED" : "FAILED") << nl;

    return 0;
}

// ************************************************************************* //
