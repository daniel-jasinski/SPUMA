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
    Foam::memoryExecutor

Description
    Base class for the device memory executors.

SourceFiles
    memoryExecutor.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_memory_Executor_H
#define Foam_memory_Executor_H

#include "label.H"
#include "scalar.H"
#include "memCopyKind.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class memoryExecutor Declaration
\*---------------------------------------------------------------------------*/

template<typename T>
class memoryExecutor
{
public:
    
    memoryExecutor(memoryExecutor<T> &other) = delete;   
    
    static inline label memAlignBytes()
    {
        return T::_memAlignBytes;
    }

    static inline void* alloc(uint64_t size)
    {
        return T::_backendAlloc(size);
    }

    static inline void clear(void* ptr)
    {
        T::_backendClear(ptr);
    }

    static inline void memCopy
    (
        void* dst,
        const void* src,
        uint64_t size,
        memCopyKind kind
    )
    {
        T::_backendMemCopy(dst, src, size, kind);
    }

    static inline void memSet
    (
        void* ptr,
        const size_t sizeInBytes,
        const void* value,
        size_t sizeOfValue
    )
    {
        T::_backendMemSet(ptr, sizeInBytes, value, sizeOfValue);
    }

    static inline void memSetScalarOne(void* ptr, const size_t sizeInBytes)
    {
       T::_backendMemSetScalarOne(ptr, sizeInBytes); 
    }

    static inline void memSet(void* ptr, const size_t sizeInBytes, const int value)
    {
        T::_backendMemSet(ptr,sizeInBytes,value);
    }

    //- operators

    void operator=(const memoryExecutor<T> &) = delete;
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
