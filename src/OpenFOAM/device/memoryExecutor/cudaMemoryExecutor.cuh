/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Class
    Foam::cudaMemoryExecutor

Description
    Cuda executor for the memory pool operations.

SourceFiles
    cudaMemoryExecutor.cuh
    cudaMemoryExecutor.cu

\*---------------------------------------------------------------------------*/

#ifndef Foam_cuda_Memory_Executor_H
#define Foam_cuda_Memory_Executor_H

#include "memoryExecutor.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class cudaMemoryExecutor Declaration
\*---------------------------------------------------------------------------*/

class cudaMemoryExecutor
: public memoryExecutor<cudaMemoryExecutor>
{
public:


    static constexpr label _memAlignBytes = 16;

    static void* _backendAlloc(uint64_t size);

    static void _backendClear(void* ptr);

    static void _backendMemCopy
    (
        void* dst,
        const void* src,
        uint64_t size,
        memCopyKind kind
    );

    static void _backendMemSet
    (
        void* ptr,
        const size_t sizeInBytes,
        const void* value,
        size_t sizeOfValue
    );

    static void _backendMemSetScalarOne
    (
       void* ptr,
       const size_t sizeInBytes
    );

    static void _backendMemSet
    (
        void* ptr,
        const size_t sizeInBytes,
        const int value
    );
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif
