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

\*---------------------------------------------------------------------------*/

#include <cstring>
#include "memoryExecutors.H"
#include "memCopyKind.H"
#include "umpireMemoryPool.H"
#include "error.H"

namespace Foam
{
    defineTypeNameAndDebug(umpireMemoryPool,  0);
}

// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

Foam::umpireMemoryPool::umpireMemoryPool(const uint64_t size):
    Foam::Spuma::MemoryPool::MemoryPool(),
    rm_(umpire::ResourceManager::getInstance()),
    inspector_()
{
    DebugInfo<<"create umpire memory pool"<<endl;
#if defined(have_cuda) || defined(have_hip)
    auto allocator = rm_.getAllocator("UM");
#else
    auto allocator = rm_.getAllocator("HOST");
#endif

    auto hostAllocator = rm_.getAllocator("HOST");

    constexpr uint64_t GB = 1024ul*1024*1024;
    initialSize_ = size >  GB ? size : GB;
    minBlockSize_ = 1024*1024; //1Mb

    allocator_ = rm_.makeAllocator<umpire::strategy::DynamicPoolList>
    (
        "dynamic_pool",
        allocator,
        initialSize_, /*default 512 Mb*/
        minBlockSize_ /*default 1Mb*/
    );

    st_ = new umpire::strategy::DynamicPoolList
    (
        "strategy",
        hostAllocator.getId(),hostAllocator
    );
};

// * * * * * * * * * * * * * * * Destructors  * * * * * * * * * * * * * * * //

Foam::umpireMemoryPool::~umpireMemoryPool()
{};

// * * * * * * * * * * * * * Public Member Functions  * * * * * * * * * * * //

void* Foam::umpireMemoryPool::allocate(uint64_t sizeInBytes)
{
    if (!sizeInBytes)
    {
        WarningInFunction<< "Trying to allocate a block of zero size." << nl;
        return nullptr;
    }

    void* poolPtr = this->allocator_.allocate(sizeInBytes);

    // update max occupancy
    const uint64_t allocatedSizeInBytes = this->allocatedSize();
    if (allocatedSizeInBytes > this->maxOccupancy_) this->maxOccupancy_ = allocatedSizeInBytes;

    return poolPtr;
};

void Foam::umpireMemoryPool::free(void* ptr)
{
    //if ptr is null do nothing
    if (ptr == nullptr) return;

    //check if pointer was allocated with pool
    rm_.findAllocationRecord(ptr);

    allocator_.deallocate(ptr);
};

uint64_t Foam::umpireMemoryPool::arraySizeInBytes(void* poolPtr)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return 0;

    //check if pointer was allocated with pool
    auto record = rm_.findAllocationRecord(poolPtr);

    return record->size;
};

void Foam::umpireMemoryPool::copyIn
(
    void* poolPtr,
    void* ptr,
    uint64_t nElementsInBytes
)
{
    // if ptr is null do nothing
    if (poolPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    // check if pointer was allocated with pool
    auto record = rm_.findAllocationRecord(poolPtr);

    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    uint64_t size = allocator_.getSize(record->ptr);
    if (nElementsInBytes > size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // workaround: umpire does not support copies between non umpire pointers
    inspector_.registerAllocation(ptr, nElementsInBytes, st_);
    rm_.copy(poolPtr, ptr, nElementsInBytes);
    inspector_.deregisterAllocation(ptr, st_);
};

void Foam::umpireMemoryPool::copyOut
(
    void* poolPtr,
    void* ptr,
    uint64_t nElementsInBytes
)
{
    // if ptr is null do nothing
    if (poolPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    // check if pointer was allocated with pool
    auto record = rm_.findAllocationRecord(poolPtr);

    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    uint64_t size = allocator_.getSize(record->ptr);
    if (nElementsInBytes > size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    inspector_.registerAllocation(ptr, nElementsInBytes, st_);
    rm_.copy(ptr, poolPtr, nElementsInBytes);
    inspector_.deregisterAllocation(ptr, st_);
};

void Foam::umpireMemoryPool::memSet
(
    void* poolPtr,
    const void* value,
    size_t sizeOfValue,
    uint64_t nElementsInBytes
)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    // check if pointer was allocated with pool
    auto record = rm_.findAllocationRecord(poolPtr);
    void* allocatedPoolPtr = record->ptr;

    const uint64_t size = allocator_.getSize(allocatedPoolPtr);
    if (reinterpret_cast<uint64_t>(poolPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedPoolPtr) + size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // use memory executor
    foamMemoryExecutor::memSet(poolPtr,nElementsInBytes,value,sizeOfValue);
};

void Foam::umpireMemoryPool::memSetScalarOne
(
    void* poolPtr,
    uint64_t nElementsInBytes
)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    // check if pointer was allocated with pool
    auto record = rm_.findAllocationRecord(poolPtr);
    void* allocatedPoolPtr = record->ptr;

    const uint64_t size = allocator_.getSize(allocatedPoolPtr);
    if (reinterpret_cast<uint64_t>(poolPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedPoolPtr) + size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // use memory executor
    foamMemoryExecutor::memSetScalarOne(poolPtr,nElementsInBytes);
};

void Foam::umpireMemoryPool::memSet
(
    void* poolPtr,
    const int value,
    uint64_t nElementsInBytes
)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return;

    //if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    // check if pointer was allocated with pool
    auto record = rm_.findAllocationRecord(poolPtr);
    void* allocatedPoolPtr = record->ptr;

    const uint64_t size = allocator_.getSize(allocatedPoolPtr);
    if (reinterpret_cast<uint64_t>(poolPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedPoolPtr) + size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // use foam memory executor
    foamMemoryExecutor::memSet(poolPtr,nElementsInBytes,value);
};

void Foam::umpireMemoryPool::memCopy
(
    void* tgtPtr,
    void* srcPtr,
    uint64_t nElementsInBytes
)
{
    // if ptr is null do nothing
    if (tgtPtr == nullptr || srcPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    // check if pointer was allocated with pool
    auto recordTgt = rm_.findAllocationRecord(tgtPtr);
    void* allocatedTgtPtr = recordTgt->ptr;
    const uint64_t tgtSizeInBytes = this->allocator_.getSize(allocatedTgtPtr);
    if (reinterpret_cast<uint64_t>(tgtPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedTgtPtr) + tgtSizeInBytes)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in target block"
            <<abort(FatalError);
    }

    // check if pointer was allocated with pool
    auto recordSrc = rm_.findAllocationRecord(srcPtr);
    void* allocatedSrcPtr = recordSrc->ptr;
    const uint64_t srcSizeInBytes = this->allocator_.getSize(allocatedSrcPtr);
    if (reinterpret_cast<uint64_t>(srcPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedSrcPtr) + srcSizeInBytes)
    {
        FatalErrorInFunction
            << "Trying to read more bytes than available in src block"
            <<abort(FatalError);
    }

    rm_.copy(tgtPtr,srcPtr,nElementsInBytes);
}

// ************************************************************************* //
