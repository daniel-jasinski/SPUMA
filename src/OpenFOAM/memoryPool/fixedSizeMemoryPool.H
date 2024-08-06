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
    fixedSizeMemoryPool

Description
    A naive implementation of a memory pool with a fixed size

SourceFiles
    MemoryPool.H
    fixedSizeMemoryPool.H
    fixedSizeMemoryPool.C
*/


#ifndef fixed_Size_Memory_Pool_Executor_H
#define fixed_Size_Memory_Pool_Executor_H

#include <iostream>
#include "MemoryPool.H"
#include "memoryPoolExecutor.H"

namespace Foam
{

class fixedSizeMemoryPool:
    public MemoryPool
{

private:

    // pointer to pool
    char* v_;
    // Executor associated with the memory Pool. (CPU, GPU, openMP, etc.)
    const memoryPoolExecutor exec_;

    // Private constructor to prevent instancing
    fixedSizeMemoryPool(const uint64_t size);

    fixedSizeMemoryPool(const dictionary& dict);

public:

    //declare interface as friend
    friend class MemoryPool;

    // Static Data
    TypeName("fixedSizeMemoryPool");

    // Class destructor
    virtual ~fixedSizeMemoryPool();

    // new function
    void* allocate(uint64_t size);

    // free function
    void free(void* ptr);

    // check if pool pointer is valid
    inline bool isValid(void* poolPtr);

    // Returns size of array (in bytes if type not specified)
    uint64_t arraySizeInBytes(void* poolPtr);

    // Copies data in pool
    void copyIn(void* poolPtr, void* ptr, uint64_t nElementsInBytes = 0, uint64_t offsetInBytes = 0);

    // Copies data from pool
    void copyOut(void* poolPtr, void* ptr, uint64_t nElementsInBytes = 0, uint64_t offsetInBytes = 0);

    // Set the memory to a value for each byte of specified memory
    void memSet
    (
        void* poolPtr,
        const void* value,
        size_t sizeOfValue,
        uint64_t nElementsInBytes = 0,
        uint64_t offsetInBytes = 0
    );

    // Set the memory to scalar one
    void memSetScalarOne
    (
        void* poolPtr,
        uint64_t nElementsInBytes = 0,
        uint64_t offsetInBytes = 0
    );

    // Set the memory to an int value using built-in memSet function
    void memSet
    (
        void* poolPtr,
        const int value,
        uint64_t nElementsInBytes = 0,
        uint64_t offsetInBytes = 0
    );

    // Copy from one block of the memory pool to another
    void memCopy
    (
        void* tgtPtr,
        void* srcPtr,
        uint64_t nElementsInBytes = 0,
        uint64_t tgtOffsetInBytes = 0,
        uint64_t srcOffsetInBytes = 0
    );

    // Returns pool size
    uint64_t size() const
    {
        return size_;
    };

    // Returns allocated size
    uint64_t allocatedSize() const
    {
        return allocatedSize_;
    };

    // Returns unallocated size
    uint64_t unallocatedSize() const
    {
        return unallocatedSize_;
    };

    // Prints list of allocated blocks
    void showAllocated(bool relative = false);

    // Prints list of unallocated blocks
    void showUnallocated(bool relative = false);

    // read memory pool properties
    void readProperties(const dictionary &typeDict);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// inline member functions

inline bool Foam::fixedSizeMemoryPool::isValid(void* poolPtr)
{
    if ( !poolPtr ) return false;

    blockList::iterator block = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    bool valid = block == this->usedBlockList_.end() ? false : true;
    if (!valid)
    {
        FatalErrorInFunction
            << "MEMPOOL: pointer " << reinterpret_cast<uint64_t>(poolPtr)
            << "is not valid" << abort(FatalError);
    }
    return valid;
}

#endif

// ************************************************************************* //

