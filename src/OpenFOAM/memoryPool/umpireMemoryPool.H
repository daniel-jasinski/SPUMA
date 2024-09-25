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
    umpireMemoryPool

Description
    wrapper for umpire library

SourceFiles
    MemoryPool.H
    umpireMemoryPool.H
    umpireMemoryPool.C
\* ---------------------------------------------------------------------------*/


#ifndef umpire_Memory_Pool_Executor_H
#define umpire_Memory_Pool_Executor_H

#include <iostream>
#include "MemoryPool.H"
#include "memoryPoolExecutor.H"
#include <umpire/ResourceManager.hpp>
#include <umpire/Allocator.hpp>
#include <umpire/strategy/DynamicPoolList.hpp>
#include <umpire/strategy/mixins/Inspector.hpp>

namespace Foam
{

class umpireMemoryPool:
    public MemoryPool
{

private:

    // umpire resource manager
    umpire::ResourceManager& rm_;

    // umpire allocator
    umpire::Allocator allocator_;

    // tmp umpire allocator to use for copy and memset ops
    umpire::Allocator tmpAllocator_;

    // umpire strategy to register/deregister allocation
    umpire::strategy::DynamicPoolList* st_;
    umpire::strategy::mixins::Inspector inspector_;

    uint64_t initialSize_;
    uint64_t minBlockSize_;

    // Private constructor to prevent instancing
    umpireMemoryPool(const uint64_t size);

    umpireMemoryPool(const dictionary& dict);

public:

    //declare interface as friend
    friend class MemoryPool;

    // Static Data
    TypeName("umpireMemoryPool");

    // Class destructor
    virtual ~umpireMemoryPool();

    // new function
    void* allocate(uint64_t size);

    // free function
    void free(void* ptr);

    // check if pool pointer is valid
    inline bool isValid(void* poolPtr);

    // check if ptr is in allocated blocks
    virtual inline bool isInRange(void* ptr)
    {
        NotImplemented;
    };
    
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

    // set the memory to scalar one
    void memSetScalarOne
    (
        void* poolPtr,
        uint64_t nElementsInBytes,
        uint64_t offsetInBytes
    );

    // Set the memory to an int value using built in memset function of umpire
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
        return allocator_.getActualSize();
    };

    // Returns allocated size
    uint64_t allocatedSize() const
    {
        return allocator_.getCurrentSize();
    };

    // Returns allocated size
    uint64_t unallocatedSize() const
    {
        return this->size() - this->allocatedSize();
    };

    // Returns occupancy rate (percent)
    scalar occupancy()
    {
        scalar occupancy = 100 * scalar(this->allocatedSize()) / scalar(this->size());
        return occupancy;
    };

    // Prints list of allocated blocks
    void showAllocated(bool relative = false)
    {
        NotImplemented;
    };

    // Prints list of unallocated blocks
    void showUnallocated(bool relative = false)
    {
        NotImplemented;
    };

    // read memory pool properties
    void readProperties(const dictionary &typeDict);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// inline member functions

inline bool Foam::umpireMemoryPool::isValid(void* poolPtr)
{
    // if poolPtr is null
    if (!poolPtr) return false;

    auto record = rm_.findAllocationRecord(poolPtr);
    bool valid = record->ptr == nullptr ? false : true;
    if (!valid)
    {
        FatalErrorInFunction
            << "MEMPOOL: pointer " << reinterpret_cast<uint64_t>(poolPtr)
            << "is not valid" << endl;
    }
    return valid;
}

#endif

// ************************************************************************* //

