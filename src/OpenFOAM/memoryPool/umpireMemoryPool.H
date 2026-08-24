/*---------------------------------------------------------------------------*\
  *      .  *_______ * ______ .  __ *  __ * ___ .___    .  ___ .   *  .     *
    *  .    /       | |   _  \  |  |  |  | |   \/   | *   /   \ *   .    *   .
 *    .  * .\   (---*.|  |_)  |.|  |  |  |*|  \  /  |. * /  *  \  .  *     *
 =^^=^^==^^^=\   \^=^=|   ___/=^|  |^=|  |=|  |\/|  |^^=/  /=\  \^=^=^^===^^^=
 0  o  O  o---)   \ 0 |  |   0  |  o--o  |o|  |  |  | o/  _____  \ 0   o  O
     0    |_______/   |__| o   o \______/  |__| 0|__| /__/  o  \__\   o
  O   o  o        0  o      0   O        o    o       O  o     0   o    0  o
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of SPUMA.

    SPUMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SPUMA is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with SPUMA.  If not, see <http://www.gnu.org/licenses/>.

Class
    umpireMemoryPool

Description
    Interface to the Umpire memory pool implementation.
    See https://github.com/LLNL/Umpire

SourceFiles
    umpireMemoryPool.H
    umpireMemoryPool.C

\* ---------------------------------------------------------------------------*/

#ifndef umpire_Memory_Pool_Executor_H
#define umpire_Memory_Pool_Executor_H

#include <iostream>
#include "MemoryPoolBase.H"
#include <umpire/ResourceManager.hpp>
#include <umpire/Allocator.hpp>
#include <umpire/strategy/DynamicPoolList.hpp>
#include <umpire/strategy/mixins/Inspector.hpp>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Class umpireMemoryPool Declaration
\*---------------------------------------------------------------------------*/

class umpireMemoryPool:
    public Spuma::MemoryPool
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
    inline bool isValid(void* poolPtr){
        WarningInFunction
        <<"Umpire performs allocation checks with findAllocationRecord"
        <<endl;
        NotImplemented;
        return false;
    };

    // check if ptr is in allocated blocks
    inline bool isInBlockRange(void* ptr){
        WarningInFunction
            <<"Umpire performs intra allocation checks with findAllocationRecord"
            <<endl;
        NotImplemented;
        return false;
    };

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

    // set the memory to scalar one
    void memSetScalarOne
    (
        void* poolPtr,
        uint64_t nElementsInBytes
    );

    // Set the memory to an int value using built in memset function of umpire
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
        return allocator_.getActualSize();
    }

    // Returns allocated size
    uint64_t allocatedSize() const
    {
        return allocator_.getCurrentSize();
    }

    // Returns allocated size
    uint64_t unallocatedSize() const
    {
        return this->size() - this->allocatedSize();
    }

    // Returns occupancy rate (percent)
    scalar occupancy()
    {
        scalar occupancy = 100 * scalar(this->allocatedSize()) / scalar(this->size());
        return occupancy;
    }

    // Prints list of allocated blocks
    void showAllocated(bool relative = false)
    {
        NotImplemented;
    }

    // Prints list of unallocated blocks
    void showUnallocated(bool relative = false)
    {
        NotImplemented;
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// inline member functions

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

