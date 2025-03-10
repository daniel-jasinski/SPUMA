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
    dummyMemoryPool

Description
    A memory pool naive implementation that does not use any pooling
    strategy.

SourceFiles
    dummyMemoryPool.H
    dummyMemoryPool.C

\* ---------------------------------------------------------------------------*/

#ifndef dummy_Memory_Pool_Executor_H
#define dummy_Memory_Pool_Executor_H

#include <iostream>
#include "MemoryPool.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Class dummyMemoryPool Declaration
\*---------------------------------------------------------------------------*/

class dummyMemoryPool:
    public MemoryPool
{

private:

    // Private constructor to prevent instancing
    dummyMemoryPool(const uint64_t size = 0);

public:

    // declare interface as friend
    friend class MemoryPool;

    // Static Data
    TypeName("dummyMemoryPool");

    // Class destructor
    virtual ~dummyMemoryPool();

    // new function
    void* allocate(uint64_t size);

    // free function
    void free(void* ptr);

    // check if pool pointer is valid
    inline bool isValid(void* poolPtr);

    //- check if ptr is in allocated blocks
    inline bool isInBlockRange(void* ptr);

    // Returns size of array (in bytes if type not specified)
    uint64_t arraySizeInBytes(void* poolPtr);

    // Copies data in pool
    void copyIn
    (
        void* poolPtr,
        void* ptr,
        uint64_t nElementsInBytes
    );

    // Copies data from pool
    void copyOut
    (
        void* poolPtr,
        void* ptr,
        uint64_t nElementsInBytes
    );

    // Set the memory to a value for each byte of specified memory
    void memSet
    (
        void* poolPtr,
        const void* value,
        size_t sizeOfValue,
        uint64_t nElementsInBytes
    );

    // Set the memory to scalar one
    void memSetScalarOne
    (
        void* poolPtr,
        uint64_t nElementsInBytes
    );
    // Set the memory to an int value using built-in memSet function
    void memSet
    (
        void* poolPtr,
        const int value,
        uint64_t nElementsInBytes
    );

    // Copy from one block of the memory pool to another
    void memCopy
    (
        void* tgtPtr,
        void* srcPtr,
        uint64_t nElementsInBytes
    );

    // Returns pool size
    uint64_t size() const
    {
        return size_;
    }

    // Returns allocated size
    uint64_t allocatedSize() const
    {
        return allocatedSize_;
    }

    // Returns unallocated size
    uint64_t unallocatedSize() const
    {
        return unallocatedSize_;
    }

    // Prints list of allocated blocks
    void showAllocated(bool relative = false);

    // Prints list of unallocated blocks
    void showUnallocated(bool relative = false);
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// inline member functions

inline bool Foam::dummyMemoryPool::isValid(void* poolPtr)
{
    //case when poolPtr is null
    if ( !poolPtr ) return false;

    blockList::iterator block = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(poolPtr)
    );
    bool valid = block == this->usedBlockList_.end() ? false : true;

    return valid;
}

inline bool Foam::dummyMemoryPool::isInBlockRange(void* ptr)
{
    // case when ptr is null
    if (!ptr) return false;

    bool inRange = false;
    uint64_t Ptr = reinterpret_cast<uint64_t>(ptr);
    
    // loop trough allocated records;
    // use reverse iterator, likely the last allocated element 
    // will be the searched element
    for (auto block = this->usedBlockList_.rbegin(); 
              block!= this->usedBlockList_.rend(); ++block)
    {
        uint64_t poolptr = reinterpret_cast<uint64_t>(block->first); 
        inRange = (Ptr >= poolptr) && (Ptr <= (poolptr + block->second));
        if(inRange)
            break;
    };

    return inRange;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

