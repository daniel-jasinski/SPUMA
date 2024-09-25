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

\*---------------------------------------------------------------------------*/

#include <cstring>
#include "memoryExecutors.H"
#include "fixedSizeMemoryPool.H"
#include "error.H"
#include "dictionary.H"

namespace Foam
{
    defineTypeNameAndDebug(fixedSizeMemoryPool, 0);
    addToRunTimeSelectionTable(MemoryPool,fixedSizeMemoryPool,dictionary);
    addToRunTimeSelectionTable(MemoryPool,fixedSizeMemoryPool,word);
}
// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

Foam::fixedSizeMemoryPool::fixedSizeMemoryPool(const uint64_t size):
    Foam::MemoryPool::MemoryPool(),
    v_(nullptr)
{
    if (size > 0)
    {
        void* ptr = foamMemoryExecutor::alloc(size);
        this->v_ = static_cast<char*>(ptr);
        this->size_ = size;
    }
    else
    {
        FatalErrorInFunction
            << "size must be a positve integer"
            << exit(FatalError);
    }

    this->unusedBlockList_.insert(blockPair(this->v_, this->size_));
    this->unallocatedSize_ = this->size_;
};

Foam::fixedSizeMemoryPool::fixedSizeMemoryPool(const Foam::dictionary& dict):
    Foam::MemoryPool::MemoryPool(dict),
    v_(nullptr)
{
    this->readProperties(dict.subDict(this->type()));

    if (size_ > 0)
    { 
        void* ptr = foamMemoryExecutor::alloc(size_);
        v_ = static_cast<char*>(ptr);
    }

    unusedBlockList_.insert(blockPair(v_, size_));
    unallocatedSize_ = size_;
};

// * * * * * * * * * * * * * * * Destructors  * * * * * * * * * * * * * * * //

Foam::fixedSizeMemoryPool::~fixedSizeMemoryPool()
{
    if (this->v_)
        foamMemoryExecutor::clear(this->v_);
};

// * * * * * * * * * * * * * Public Member Functions  * * * * * * * * * * * //

void* Foam::fixedSizeMemoryPool::allocate(uint64_t size)
{
    uint64_t sizeAvailable;

    label memAlignBytes = foamMemoryExecutor::memAlignBytes();

    uint64_t sizeAligned = memAlignBytes*((size + memAlignBytes - 1)/memAlignBytes);

    if (!size)
    {
        WarningInFunction<< "Trying to allocate a block of zero size." << nl;
        return nullptr;
    }

    for (blockList::iterator ii=this->unusedBlockList_.begin(); ii!=this->unusedBlockList_.end(); ++ii)
    {
        sizeAvailable = ii->second;
        if (sizeAligned <= sizeAvailable)
        {
            char* head = ii->first;
            this->usedBlockList_.insert(blockPair(head, size));

            DebugInFunction
                << "Allocated block of size "
                << size << " at address "<< reinterpret_cast<uint64_t>(head) << nl;

            this->unusedBlockList_.erase(head);

            if (sizeAvailable - sizeAligned > 0)
                this->unusedBlockList_.insert(blockPair(head + sizeAligned, sizeAvailable - sizeAligned));

            this->allocatedSize_   += sizeAligned;
            this->unallocatedSize_ -= sizeAligned;

            uint64_t totOccupancy = head + sizeAligned - this->v_;

            if (totOccupancy > this->maxOccupancy_)
            {
                this->maxOccupancy_ = totOccupancy;
            }

            return (void*)head;
        }
    }

    FatalErrorInFunction
        << "No large enough block in memory in pool for block size " << size
        << exit(FatalError);

    return nullptr; //avoid return warning
};

// free function
void Foam::fixedSizeMemoryPool::free(void* ptr)
{
    //if ptr is null do nothing
    if (ptr == nullptr) return;
    //check if pointer was allocated with pool
    if (!this->isValid(ptr)){
        raisePoolValidError(ptr)
    }

    blockList::iterator block = this->usedBlockList_.find(reinterpret_cast<char*>(ptr));

    label memAlignBytes = foamMemoryExecutor::memAlignBytes();

    uint64_t sizeAligned = memAlignBytes*((block->second + memAlignBytes - 1)/memAlignBytes);

    this->unusedBlockList_.insert(blockPair(block->first, sizeAligned));
    DebugInFunction
        << "Deallocated block of size " << sizeAligned
        << " at address " << reinterpret_cast<uint64_t>(ptr) << nl;

    this->allocatedSize_   -= sizeAligned;
    this->unallocatedSize_ += sizeAligned;

    this->usedBlockList_.erase(reinterpret_cast<char*>(ptr));
    block = this->unusedBlockList_.find(reinterpret_cast<char*>(ptr));

    // coalesce logic
    // Check if previous empty block is contiguous
    if (block != this->unusedBlockList_.begin())
    {
        blockList::iterator block_previous = block;
        block_previous--;
        if (block_previous->first + block_previous->second == block->first)
        {
            block_previous->second += block->second;
            this->unusedBlockList_.erase(block->first);
            block = block_previous;
        }
    }

    // Check if following empty block is contiguous
    if (block != this->unusedBlockList_.end())
    {
        blockList::iterator block_following = block;
        block_following++;
        if (block_following != this->unusedBlockList_.end())
        {
            if (block->first + block->second == block_following->first)
            {
                block->second += block_following->second;
                this->unusedBlockList_.erase(block_following->first);
            }
        }
    }
};

// Returns size of array (in bytes if type not specified)
uint64_t Foam::fixedSizeMemoryPool::arraySizeInBytes(void* poolPtr)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr)){
        raisePoolValidError(poolPtr)
    }

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    return mapElement->second;
};

void Foam::fixedSizeMemoryPool::copyIn(void* poolPtr, void* ptr, uint64_t nElementsInBytes, uint64_t offsetInBytes)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr)){
        raisePoolValidError(poolPtr)
    }
    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= sizeInBytes)
        sizeInBytes = nElementsInBytes;

    foamMemoryExecutor::memCopy(poolPtr, ptr, sizeInBytes, memCopyKind::memCopyHostToDevice);
};

void Foam::fixedSizeMemoryPool::copyOut(void* poolPtr, void* ptr, uint64_t nElementsInBytes, uint64_t offsetInBytes)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr)){
        raisePoolValidError(poolPtr)
    }
    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= sizeInBytes)
        sizeInBytes = nElementsInBytes;

    foamMemoryExecutor::memCopy(ptr, poolPtr, sizeInBytes, memCopyKind::memCopyDeviceToHost);
};

void Foam::fixedSizeMemoryPool::memSet
(
    void* poolPtr,
    const void* value,
    size_t sizeOfValue,
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr)){
        raisePoolValidError(poolPtr)
    }

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(sizeInBytes))
        sizeInBytes = nElementsInBytes;

    foamMemoryExecutor::memSet(poolPtr, sizeInBytes, value, sizeOfValue);
}

void Foam::fixedSizeMemoryPool::memSetScalarOne
(
    void* poolPtr,
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr)){
        raisePoolValidError(poolPtr)
    }
    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(sizeInBytes))
        sizeInBytes = nElementsInBytes;

    foamMemoryExecutor::memSetScalarOne(poolPtr, sizeInBytes);
}

void Foam::fixedSizeMemoryPool::memSet
(
    void* poolPtr,
    const int value,
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr)){
        raisePoolValidError(poolPtr)
    }

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(sizeInBytes))
        sizeInBytes = nElementsInBytes;

    foamMemoryExecutor::memSet(poolPtr, sizeInBytes, value);
}


void Foam::fixedSizeMemoryPool::memCopy
(
    void* tgtPtr,
    void* srcPtr,
    uint64_t nElementsInBytes,
    uint64_t tgtOffsetInBytes,
    uint64_t srcOffsetInBytes
)
{

    if (!this->isValid(tgtPtr)){
        raisePoolValidError(tgtPtr)
        return;
    }
    blockList::iterator tgtElement = this->usedBlockList_.find(reinterpret_cast<char*>(tgtPtr));

    void* allocatedSrcPtr = srcPtr;
    if (!this->isValid(srcPtr)){
        //find nearest valid pointer
        if(!this->isInBlockRange(srcPtr)){
            FatalErrorInFunction
                << "MEMPOOL: src pointer " << reinterpret_cast<uint64_t>(srcPtr)
                << "is not valid and not in range" << abort(FatalError);
        }
        uint64_t ptr = reinterpret_cast<uint64_t>(srcPtr);    
        
        for (auto block = this->usedBlockList_.rbegin(); block!= this->usedBlockList_.rend(); ++block)
        {
            uint64_t allocatedPtr = reinterpret_cast<uint64_t>(block->first);
            if ( (ptr>allocatedPtr) && (ptr < allocatedPtr + block->second) ){
                allocatedSrcPtr = reinterpret_cast<void*>(allocatedPtr);
                break;
            }
        }
    }
    blockList::iterator srcElement = this->usedBlockList_.find(reinterpret_cast<char*>(allocatedSrcPtr));

    srcPtr = (char*)srcPtr + srcOffsetInBytes;
    tgtPtr = (char*)tgtPtr + tgtOffsetInBytes;
    uint64_t srcSizeInBytes = srcElement->second - srcOffsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= srcSizeInBytes)
        srcSizeInBytes = nElementsInBytes;

    uint64_t tgtSizeInBytes = tgtElement->second - tgtOffsetInBytes;
    if (tgtSizeInBytes < srcSizeInBytes)
    {
        FatalErrorInFunction
            << " Not enough space in target block " << nl;
        return;
    }

    foamMemoryExecutor::memCopy(tgtPtr, srcPtr, srcSizeInBytes, memCopyKind::memCopyDeviceToDevice);
}

void Foam::fixedSizeMemoryPool::showAllocated(bool relative)
{
    uint64_t offset = 0;
    if (relative)
        offset = reinterpret_cast<uint64_t>(this->v_);
    for ( blockList::iterator ii = this->usedBlockList_.begin(); ii != this->usedBlockList_.end(); ii++ )
        Info
            << "At address: "<< reinterpret_cast<uint64_t>(ii->first) - offset
            << " allocated block of size " << ii->second << " bytes." << nl;
}

void Foam::fixedSizeMemoryPool::showUnallocated(bool relative)
{
    uint64_t offset = 0;
    if (relative)
        offset = reinterpret_cast<uint64_t>(this->v_);
    for ( blockList::iterator ii = this->unusedBlockList_.begin(); ii != this->unusedBlockList_.end(); ii++ )
        Info
            << "At address: "<< reinterpret_cast<uint64_t>(ii->first) - offset
            << " allocated block of size " << ii->second << " bytes." << nl;
};

// read memory pool properties
void Foam::fixedSizeMemoryPool::readProperties(const dictionary &typeDict)
{
    constexpr uint64_t MB = 1024*1024ul;
    size_ = typeDict.get<uint64_t>("size")*MB;
}
