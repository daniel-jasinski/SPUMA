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
    memoryPoolTest

Group
    test/device 

Description
    Unit test to check memory pool operations.

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
        "Test for memory pool operations"
    );

    #include "setRootCaseLists.H"
    #include "initDevice.H"
    #include "createMemoryPool.H"

    Info << "Testing Memory pool with low-level allocate/free operations" << endl << endl;

    Info << "Pool occupancy before allocations: " << memPool->occupancy() << nl;
    Info << "Pool allocated size: " << memPool->allocatedSize() << " bytes" << nl;
    Info << "Pool total size: " << memPool->size() << " bytes" << nl;

    // Test 1: Allocate and use scalar array
    
    Info << nl << "Test 1: Allocating scalar array for CellCenters (1024 elements):" << nl;

    const label nScalars = 1024;
    scalar* cellCentres_ptr = static_cast<scalar*>(memPool->allocate(nScalars * sizeof(scalar)));

    Info << "\tAllocated " << (nScalars * sizeof(scalar)) << " bytes for scalars" << nl;
    printf("\tAllocated at address: %llu\n", (unsigned long long) cellCentres_ptr);
    Info << "\tPool occupancy: " << memPool->occupancy() << "%" << nl;
    Info << "\tPool allocated size: " << memPool->allocatedSize() << " bytes" << nl;

    // Initialize scalar data (2.5 to 4.0 range)
    for (int ii = 0; ii < nScalars; ii++) 
    {
        cellCentres_ptr[ii] = ((double) ii) * 0.000977517106549364614 * 1.5 + 2.5;
    }
    
    Info << "\tFirst 5 values: " << cellCentres_ptr[0] << " " << cellCentres_ptr[1] << " " 
         << cellCentres_ptr[2] << " " << cellCentres_ptr[3] << " " << cellCentres_ptr[4] << nl;
    
    // Validation check for scalar array
    scalar expectedScalarValues[5] = 
    {
        2.5, 
	2.501466275659780492, 
	2.502932551319561100, 
	2.504398826979341593, 
	2.505865102639122201
    };
    
    bool scalarTestPassed = true;
    const scalar tolerance = 1e-12;
    
    Info << "\tValidating scalar values against expected hardcoded values:" << nl;
    
    for (int ii = 0; ii < 5; ii++) 
    {
        scalar diff = std::abs(cellCentres_ptr[ii] - expectedScalarValues[ii]);
        if (diff > tolerance) 
        {
            scalarTestPassed = false;
            Info << "\t\tFAILED: Index " << ii << " expected=" << expectedScalarValues[ii] 
                 << " actual=" << cellCentres_ptr[ii] << " diff=" << diff << nl;
        }
        else 
        {
            Info << "\t\tPASSED: Index " << ii << " value=" << cellCentres_ptr[ii] << nl;
        }
    }
    Info << "\tScalar array validation: " << (scalarTestPassed ? "PASSED" : "FAILED") << nl;

    // Test 2: Allocate and use double array
    
    Info << "\nTest 2: Allocating double array for FaceCenters (1024 elements):" << nl;
    
    const label nDoubles = 1024;
    double* FaceCenters_ptr = static_cast<double*>(memPool->allocate(nDoubles * sizeof(double)));
    
    Info << "\tAllocated " << (nDoubles * sizeof(double)) << " bytes for doubles" << nl;
    printf("\tAllocated at address: %llu\n", (unsigned long long) FaceCenters_ptr);
    Info << "\tPool occupancy: " << memPool->occupancy() << "%" << nl;
    Info << "\tPool allocated size: " << memPool->allocatedSize() << " bytes" << nl;
    
    // Initialize double data (1.0 to 4.5 range)
    for (int ii = 0; ii < nDoubles; ii++) 
    {
        FaceCenters_ptr[ii] = ((double) ii) * 0.000977517106549364614 * 3.5 + 1.0;
    }
    
    Info << "\tFirst 5 values: " << FaceCenters_ptr[0] << " " << FaceCenters_ptr[1] << " " 
         << FaceCenters_ptr[2] << " " << FaceCenters_ptr[3] << " " << FaceCenters_ptr[4] << nl;
    
    // Validation check for double array
    double expectedDoubleValues[5] = 
    {
        1.0, 
	1.003421309722929276, 
	1.006842619445858552, 
	1.010263929168787828, 
	1.013685238891717104
    };
    
    bool doubleTestPassed = true;
    const double doubleTolerance = 1e-9;
    
    Info << "\tValidating double values against expected hardcoded values:" << nl;
    
    for (int ii = 0; ii < 5; ii++) 
    {
        double diff = std::abs(FaceCenters_ptr[ii] - expectedDoubleValues[ii]);
        if (diff > doubleTolerance) 
        {
            doubleTestPassed = false;
            Info << "\t\tFAILED: Index " << ii << " expected=" << expectedDoubleValues[ii] 
                 << " actual=" << FaceCenters_ptr[ii] << " diff=" << diff << nl;
        }
        else 
        {
            Info << "\t\tPASSED: Index " << ii << " value=" << FaceCenters_ptr[ii] << nl;
        }
    }
    
    Info << "\tDouble array validation: " << (doubleTestPassed ? "PASSED" : "FAILED") << nl;

    // Test 3: Allocate and use char array
    
    Info << "\nTest 3: Allocating char array for U field (1024 elements):" << nl;
    
    const label nChars = 1024;
    char* U_ptr = static_cast<char*>(memPool->allocate(nChars * sizeof(char)));
    
    Info << "\tAllocated " << (nChars * sizeof(char)) << " bytes for chars" << nl;
    printf("\tAllocated at address: %llu\n", (unsigned long long) U_ptr);
    Info << "\tPool occupancy: " << memPool->occupancy() << "%" << nl;
    Info << "\tPool allocated size: " << memPool->allocatedSize() << " bytes" << nl;
    
    // Initialize char data (alphabet pattern)
    for (int ii = 0; ii < nChars; ii++) 
    {
        U_ptr[ii] = (char) ((ii % 26) + 65);
    }
    
    Info << "\tFirst 10 chars: ";
    
    for (int ii = 0; ii < 10; ii++) Info << U_ptr[ii] << " ";
    
    Info << nl;
    
    // Validation check for char array
    char expectedCharValues[10] = {'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J'};
    bool charTestPassed = true;
    
    Info << "\tValidating char values against expected hardcoded values:" << nl;
    
    for (int ii = 0; ii < 10; ii++) 
    {
        if (U_ptr[ii] != expectedCharValues[ii]) 
        {
            charTestPassed = false;
            Info << "\t\tFAILED: Index " << ii << " expected=" << expectedCharValues[ii] 
                 << " actual=" << U_ptr[ii] << nl;
        }
        else 
        {
            Info << "\t\tPASSED: Index " << ii << " value=" << U_ptr[ii] << nl;
        }
    }
    
    Info << "\tChar array validation: " << (charTestPassed ? "PASSED" : "FAILED") << nl;

    // Test 4: Allocate and use label array
    
    Info << "\nTest 4: Allocating label array for p field (1024 elements):" << nl;
    
    const label nLabels = 1024;
    label* p_ptr = static_cast<label*>(memPool->allocate(nLabels * sizeof(label)));
    
    Info << "\tAllocated " << (nLabels * sizeof(label)) << " bytes for labels" << nl;
    printf("\tAllocated at address: %llu\n", (unsigned long long) p_ptr);
    Info << "\tPool occupancy: " << memPool->occupancy() << "%" << nl;
    Info << "\tPool allocated size: " << memPool->allocatedSize() << " bytes" << nl;
    
    // Initialize label data (2048 to 4094 range)
    for (int ii = 0; ii < nLabels; ii++) 
    {
        p_ptr[ii] = 2048 + 2 * ii;
    }
    
    Info << "\tFirst 5 values: " << p_ptr[0] << " " << p_ptr[1] << " " 
         << p_ptr[2] << " " << p_ptr[3] << " " << p_ptr[4] << nl;
    
    // Validation check for label array
    label expectedLabelValues[5] = {2048, 2050, 2052, 2054, 2056};
    bool labelTestPassed = true;
    
    Info << "\tValidating label values against expected hardcoded values:" << nl;
    
    for (int ii = 0; ii < 5; ii++) 
    {
        if (p_ptr[ii] != expectedLabelValues[ii]) 
        {
            labelTestPassed = false;
            Info << "\t\tFAILED: Index " << ii << " expected=" << expectedLabelValues[ii] 
                 << " actual=" << p_ptr[ii] << nl;
        }
        else 
        {
            Info << "\t\tPASSED: Index " << ii << " value=" << p_ptr[ii] << nl;
        }
    }
    
    Info << "\tLabel array validation: " << (labelTestPassed ? "PASSED" : "FAILED") << nl;

    // Test validation
    Info << "\nTest 5: Pointer validation:" << nl;
    Info << "\tcellCentres_ptr valid: " << (memPool->isValid(cellCentres_ptr) ? "YES" : "NO") << nl;
    Info << "\tFaceCenters_ptr valid: " << (memPool->isValid(FaceCenters_ptr) ? "YES" : "NO") << nl;
    Info << "\tU_ptr valid: " << (memPool->isValid(U_ptr) ? "YES" : "NO") << nl;
    Info << "\tp_ptr valid: " << (memPool->isValid(p_ptr) ? "YES" : "NO") << nl;

    // Test memory operations
    Info << "\nTest 6: Memory operations (memSet):" << nl;
    
    scalar* grad_ptr = static_cast<scalar*>(memPool->allocate(nScalars * sizeof(scalar)));
    memPool->memSetScalarOne(grad_ptr, nScalars * sizeof(scalar));
    
    Info << "\tInitialized grad_ptr with scalar ones" << nl;
    Info << "\tFirst 5 values after memSetScalarOne: " << grad_ptr[0] << " " << grad_ptr[1] << " " 
         << grad_ptr[2] << " " << grad_ptr[3] << " " << grad_ptr[4] << nl;
    
    // Validation check for memSetScalarOne operation
    scalar expectedOnesValue = 1.0;
    bool memSetTestPassed = true;
    
    Info << "\tValidating memSetScalarOne values against expected hardcoded values:" << nl;
    
    for (int ii = 0; ii < 5; ii++) 
    {
        scalar diff = std::abs(grad_ptr[ii] - expectedOnesValue);
        if (diff > tolerance) 
        {
            memSetTestPassed = false;
            Info << "\t\tFAILED: Index " << ii << " expected=" << expectedOnesValue 
                 << " actual=" << grad_ptr[ii] << " diff=" << diff << nl;
        }
        else 
        {
            Info << "\t\tPASSED: Index " << ii << " value=" << grad_ptr[ii] << nl;
        }
    }
    
    Info << "\tmemSetScalarOne validation: " << (memSetTestPassed ? "PASSED" : "FAILED") << nl;

    // Test memory copy
    Info << "\nTest 7: Memory copy operations:" << nl;
    
    scalar* grad2_ptr = static_cast<scalar*>(memPool->allocate(512 * sizeof(scalar)));
    memPool->memCopy(grad2_ptr, cellCentres_ptr, 512 * sizeof(scalar));
    
    Info << "\tCopied first 512 elements from cellCentres_ptr to grad2_ptr" << nl;
    Info << "\tFirst 5 values in grad2_ptr: " << grad2_ptr[0] << " " << grad2_ptr[1] << " " 
         << grad2_ptr[2] << " " << grad2_ptr[3] << " " << grad2_ptr[4] << nl;
    
    // Validation check for memCopy operation
    bool memCopyTestPassed = true;
    
    Info << "\tValidating memCopy operation by comparing copied values:" << nl;
    
    for (int ii = 0; ii < 5; ii++) 
    {
        scalar diff = std::abs(grad2_ptr[ii] - cellCentres_ptr[ii]);
        if (diff > tolerance) 
        {
            memCopyTestPassed = false;
            Info << "\t\tFAILED: Index " << ii << " source=" << cellCentres_ptr[ii] 
                 << " copied=" << grad2_ptr[ii] << " diff=" << diff << nl;
        }
        else 
        {
            Info << "\t\tPASSED: Index " << ii << " source=" << cellCentres_ptr[ii] 
                 << " copied=" << grad2_ptr[ii] << nl;
        }
    }
    
    Info << "\tmemCopy validation: " << (memCopyTestPassed ? "PASSED" : "FAILED") << nl;

    // Cleanup
    Info << "\nTest 8: Cleanup (freeing allocated memory):" << nl;
    memPool->free(cellCentres_ptr);
    
    Info << "\tFreed cellCentres_ptr, pool occupancy: " << memPool->occupancy() << "%" << nl;
    memPool->free(FaceCenters_ptr);
    
    Info << "\tFreed FaceCenters_ptr, pool occupancy: " << memPool->occupancy() << "%" << nl;
    memPool->free(U_ptr);
    
    Info << "\tFreed U_ptr, pool occupancy: " << memPool->occupancy() << "%" << nl;
    memPool->free(p_ptr);
    
    Info << "\tFreed p_ptr, pool occupancy: " << memPool->occupancy() << "%" << nl;
    memPool->free(grad_ptr);
    
    Info << "\tFreed grad_ptr, pool occupancy: " << memPool->occupancy() << "%" << nl;
    memPool->free(grad2_ptr);
    
    Info << "\tFreed grad2_ptr, pool occupancy: " << memPool->occupancy() << "%" << nl;
    Info << "\nFinal pool state:" << nl;
    Info << "\tPool occupancy: " << memPool->occupancy() << "%" << nl;
    Info << "\tPool allocated size: " << memPool->allocatedSize() << " bytes" << nl;
    Info << "\tPool max occupancy reached: " << memPool->maxOccupancy() << " bytes" << nl;

    // Summary of all validation tests
    Info << "\n=== memoryPoolTest execution summary: ===" << nl;
    Info << "\tScalar array test: " << (scalarTestPassed ? "PASSED" : "FAILED") << nl;
    Info << "\tDouble array test: " << (doubleTestPassed ? "PASSED" : "FAILED") << nl;
    Info << "\tChar array test: " << (charTestPassed ? "PASSED" : "FAILED") << nl;
    Info << "\tLabel array test: " << (labelTestPassed ? "PASSED" : "FAILED") << nl;
    Info << "\tmemSetScalarOne test: " << (memSetTestPassed ? "PASSED" : "FAILED") << nl;
    Info << "\tmemCopy test: " << (memCopyTestPassed ? "PASSED" : "FAILED") << nl;
    
    return 0;
}

// ************************************************************************* //
