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
    cpuMemoryPoolExecutor

Description
    cpu Visitor of the fixedSizeMemoryPool class

SourceFiles
    cpuMemoryPoolExecutor.H
    cpuMemoryPoolExecutor.C
*/


#ifndef cpu_Memory_Pool_Executor_H
#define cpu_Memory_Pool_Executor_H

#include <iostream>
#include "memCopyKind.H"
#include "error.H"

namespace Foam
{

class cpuMemoryPoolExecutor
{
public:

    cpuMemoryPoolExecutor() {};

    ~cpuMemoryPoolExecutor() {};

    label memAlignBytes() const
    {
        return 8;
    }

    void* alloc(uint64_t size) const
    {
        return new char[size];
    };

    void clear(void* ptr) const
    {
        delete [] static_cast<char*>(ptr);
    };

    void memCopy(void* dst, const void* src, uint64_t size, memCopyKind kind) const
    {
        std::memcpy(dst, src, static_cast<size_t>(size));
    };

    void memSet(void* ptr, const uint64_t sizeInBytes, const void* value, size_t sizeOfValue) const
    {
        char* tmpPtr = static_cast<char*>(ptr);
        const char* tmpValue = static_cast<const char*>(value);
        for (size_t i = 0; i < sizeInBytes; i+=sizeOfValue)
        {
            for (size_t j = 0; j < sizeOfValue; j++ )
            {
                tmpPtr[i+j] = tmpValue[j];
            }
        }
    };

    void memSetScalarOne(void* ptr, const size_t sizeInBytes) const
    {
        scalar* tmpPtr = static_cast<scalar*>(ptr);
        for (size_t i = 0; i < sizeInBytes/sizeof(scalar); i++)
        {
            tmpPtr[i] = 1.0;
        }
    };

    void memSet(void* ptr, const uint64_t sizeInBytes, const int value) const
    {
        std::memset(ptr, value, sizeInBytes);
    };

};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

