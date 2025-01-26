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
    MemoryPool

Description
    A naive implementation of a memory pool

SourceFiles
    MemoryPool.H

\*---------------------------------------------------------------------------*/

#ifndef Memory_Pool_H
#define Memory_Pool_H

#include "label.H"
#include "word.H"
#include "scalar.H"
#include "typeInfo.H"

#include <map>

//- macro to raise poolptr errors
#define raisePoolValidError(poolPtr)\
    FatalErrorInFunction\
        << "MEMPOOL: pointer " << reinterpret_cast<uint64_t>(poolPtr)\
        << "is not valid" << abort(FatalError);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
namespace Foam
{
    class MemoryPool;

/*---------------------------------------------------------------------------*\
                           Class MemoryPool Declaration
\*---------------------------------------------------------------------------*/

class MemoryPool
{
private:
    // Unique instance of this class
    static MemoryPool* instance;

protected:

    uint64_t size_;
    uint64_t allocatedSize_;
    uint64_t unallocatedSize_;
    uint64_t maxOccupancy_;

    typedef std::map<char*, uint64_t> blockList;
    typedef std::pair<char*, uint64_t> blockPair;

    blockList unusedBlockList_;
    blockList usedBlockList_;

public:
    // Static Data
    TypeName("MemoryPool");

    // Constructor
    MemoryPool():
        size_(0),
        allocatedSize_(0),
        unallocatedSize_(0),
        maxOccupancy_(0)
    {};

    MemoryPool(const dictionary& dict);

    // No copy constructor
    MemoryPool(MemoryPool &other) = delete;

    // Selectors
    static MemoryPool* New
    (
        const word& type,
        const uint64_t size
    );


    // Destructor
    virtual ~MemoryPool();

    // new function
    virtual void* allocate(uint64_t size) = 0;

    // free function
    virtual void free(void* ptr) = 0;

    // check if pool pointer is valid
    virtual inline bool isValid(void* poolPtr) = 0;

    // check if ptr is in allocated blocks
    virtual inline bool isInBlockRange(void* ptr) = 0;

    // Returns size of array (in bytes if type not specified)
    virtual uint64_t arraySizeInBytes(void* poolPtr) = 0;

    // Copies data in pool
    virtual void copyIn(void* poolPtr, void* ptr, uint64_t nElementsInBytes = 0, uint64_t offsetInBytes = 0) = 0;

    // Copies data from pool
    virtual void copyOut(void* poolPtr, void* ptr, uint64_t nElementsInBytes = 0, uint64_t offsetInBytes = 0) = 0;

    // Set the memory to a value for each byte of specified memory
    virtual void memSet
    (
        void* poolPtr,
        const void* value,
        size_t sizeOfValue,
        uint64_t nElementsInBytes = 0,
        uint64_t offsetInBytes = 0
    ) = 0;

    // Set the memory to a value of one fo a scalar type for each byte of specified memory
    virtual void memSetScalarOne
    (
        void* poolPtr,
        uint64_t nElementsInBytes = 0,
        uint64_t offsetInBytes = 0
    ) = 0;

    // Set the memory to an int value using built-in memset function
    virtual void memSet
    (
        void* poolPtr,
        const int value,
        uint64_t nElementsInBytes = 0,
        uint64_t offsetInBytes = 0
    ) = 0;

    // Copy from one block of the memory pool to another
    virtual void memCopy
    (
        void* tgtPtr,
        void* srcPtr,
        uint64_t nElementsInBytes = 0,
        uint64_t tgtOffsetInBytes = 0,
        uint64_t srcOffsetInBytes = 0
    ) = 0;

    // Returns pool size
    virtual uint64_t size() const = 0;

    // Returns allocated size
    virtual uint64_t allocatedSize() const = 0;

    // Returns allocated size
    virtual uint64_t unallocatedSize() const = 0;

    // Returns occupancy rate (percent)
    virtual scalar occupancy() const
    {
        return 100 * scalar(this->allocatedSize()) / scalar(this->size());
    };

    // Returns max occupancy reached
    virtual uint64_t maxOccupancy() const
    {
        return maxOccupancy_;
    };

    // Prints list of allocated blocks
    virtual void showAllocated(bool relative = false) = 0;

    // Prints list of unallocated blocks
    virtual void showUnallocated(bool relative = false) = 0;

    /* Static access method. */
    static MemoryPool* getInstance();

    // operators

    // remove copy assignement operator
    virtual void operator=(const MemoryPool &) = delete;

};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
