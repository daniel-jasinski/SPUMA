/*---------------------------------------------------------------------------*\
-------------------------------------------------------------------------------
    Copyright (C) 2023-2024 CINECA
-------------------------------------------------------------------------------
License
    This file is part of zeptoFOAM.

    zeptoFOAM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    zeptoFOAM is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with zeptoFOAM.
    If not, see <http://www.gnu.org/licenses/>.

Class
    cudaManagedMemoryPoolExecutor

Description
    cuda Visitor of the fixedSizeMemoryPool class

SourceFiles
    cudaManagedMemoryPoolExecutor.H
    cudaManagedMemoryPoolExecutor.C
*/


#ifndef cuda_Managed_Memory_Pool_Executor_H
#define cuda_Managed_Memory_Pool_Executor_H

#include "label.H"

#include "memCopyKind.H"


namespace Foam
{

class cudaManagedMemoryPoolExecutor
{
public:

    cudaManagedMemoryPoolExecutor() {};

    ~cudaManagedMemoryPoolExecutor() {};

    label memAlignBytes() const
    {
        return 16;
    }

    void* alloc(uint64_t size) const;

    void clear(void* ptr) const;

    void memCopy(void* dst, const void* src, uint64_t size, memCopyKind kind) const;

    void memSet(void* ptr, const size_t sizeInBytes, const void* value, size_t sizeOfValue) const;

    void memSetScalarOne(void* ptr, const size_t sizeInBytes) const;

    void memSet(void* ptr, const size_t sizeInBytes, const int value) const;
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

