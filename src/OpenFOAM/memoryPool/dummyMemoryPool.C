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
\* ---------------------------------------------------------------------------*/

#include <cstring>
#include "memCopyKind.H"
#include "dummyMemoryPool.H"
#include "error.H"

namespace Foam
{
    defineTypeNameAndDebug(dummyMemoryPool, 0);
    addToRunTimeSelectionTable(MemoryPool,dummyMemoryPool,dictionary);
    addToRunTimeSelectionTable(MemoryPool,dummyMemoryPool,word);
}
// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

Foam::dummyMemoryPool::dummyMemoryPool(const uint64_t size):
    Foam::MemoryPool::MemoryPool(),
#ifdef have_cuda
        #ifdef have_managed
        exec_(Foam::cudaManagedMemoryPoolExecutor())
        #else
        exec_(Foam::cudaMemoryPoolExecutor())
        #endif
#else
        exec_(Foam::cpuMemoryPoolExecutor())
#endif
{
    DebugInFunction << "MEMPOOL: using dummy memory Pool " << nl;
};

Foam::dummyMemoryPool::dummyMemoryPool(const Foam::dictionary& dict):
    Foam::MemoryPool::MemoryPool(dict),
#ifdef have_cuda
        #ifdef have_managed
        exec_(Foam::cudaManagedMemoryPoolExecutor())
        #else
        exec_(Foam::cudaMemoryPoolExecutor())
        #endif
#else
        exec_(Foam::cpuMemoryPoolExecutor())
#endif
{
    DebugInFunction << "MEMPOOL: using dummy memory Pool" << nl;
}
// * * * * * * * * * * * * * * * Destructors  * * * * * * * * * * * * * * * //

Foam::dummyMemoryPool::~dummyMemoryPool()
{
    DebugInFunction << "MEMPOOL: destroy dummy memory Pool"<<nl;
};

// * * * * * * * * * * * * * Public Member Functions  * * * * * * * * * * * //

void* Foam::dummyMemoryPool::allocate(uint64_t size)
{
    void* head;
    std::visit([this, &head, size](const auto& exec)
               { head = exec.alloc(size); },
               exec_);

    usedBlockList_.insert(blockPair(static_cast<char*>(head),size));
    this->size_ += size;
    this->allocatedSize_ += size;

    uint64_t totOccupancy = 0;
    for (auto &&block : usedBlockList_)
    {
        totOccupancy += block.second;
    }

    this->maxOccupancy_ =
        totOccupancy > this->maxOccupancy_ ? totOccupancy : this->maxOccupancy_;

    return head;

};

// free function
void Foam::dummyMemoryPool::free(void* ptr)
{
    //if ptr is null do nothing
    if (ptr == nullptr) return;
    //check if pointer was allocated with pool
    if (!this->isValid(ptr))
        return;

    blockList::iterator block = this->usedBlockList_.find(reinterpret_cast<char*>(ptr));

    std::visit([this,&ptr](const auto& exec)
                   { exec.clear(ptr); },
                   exec_);

    uint64_t size = block->second;
    this->unusedBlockList_.insert(blockPair(block->first, size));
    DebugInFunction
        << "Deallocated block of size " << size
        << " at address " << reinterpret_cast<uint64_t>(ptr) << nl;

    this->allocatedSize_   -= size;
    this->size_            -= size;
    this->unallocatedSize_ += size;

    this->usedBlockList_.erase(reinterpret_cast<char*>(ptr));

};

// Returns size of array (in bytes if type not specified)
uint64_t Foam::dummyMemoryPool::arraySizeInBytes(void* poolPtr)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return 0;

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    return mapElement->second;

};

void Foam::dummyMemoryPool::copyIn(void* poolPtr, void* ptr, uint64_t nElementsInBytes, uint64_t offsetInBytes)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;
    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;

    if (nElementsInBytes != 0 && nElementsInBytes <= sizeInBytes)
        sizeInBytes = nElementsInBytes;

    std::visit([this, poolPtr, ptr, sizeInBytes](const auto& exec)
                { exec.memCopy(poolPtr, ptr, sizeInBytes, memCopyKind::memCopyHostToDevice); },
                exec_);

};

void Foam::dummyMemoryPool::copyOut(void* poolPtr, void* ptr, uint64_t nElementsInBytes, uint64_t offsetInBytes)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;
    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;

    if (nElementsInBytes != 0 && nElementsInBytes <= sizeInBytes)
        sizeInBytes = nElementsInBytes;

    std::visit([this, ptr, poolPtr, sizeInBytes](const auto& exec)
                { exec.memCopy(ptr, poolPtr, sizeInBytes, memCopyKind::memCopyDeviceToHost); },
                exec_);

};

void Foam::dummyMemoryPool::memSet
(
    void* poolPtr,
    const void* value,
    size_t sizeOfValue,
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;

    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(sizeInBytes))
        sizeInBytes = nElementsInBytes;

    std::visit([this, poolPtr, sizeInBytes, value, sizeOfValue](const auto& exec)
                { exec.memSet(poolPtr, sizeInBytes, value, sizeOfValue); },
                exec_);

}

void Foam::dummyMemoryPool::memSetScalarOne
(
    void* poolPtr,
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;

    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(sizeInBytes))
        sizeInBytes = nElementsInBytes;

    std::visit([this, poolPtr, sizeInBytes](const auto& exec)
                { exec.memSetScalarOne(poolPtr, sizeInBytes); },
                exec_);

}

void Foam::dummyMemoryPool::memSet
(
    void* poolPtr,
    const int value,
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    blockList::iterator mapElement = this->usedBlockList_.find(reinterpret_cast<char*>(poolPtr));
    poolPtr = (char*)poolPtr + offsetInBytes;
    uint64_t sizeInBytes = mapElement->second - offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(sizeInBytes))
        sizeInBytes = nElementsInBytes;

    std::visit([this, poolPtr, sizeInBytes, value](const auto& exec)
                { exec.memSet(poolPtr, sizeInBytes, value); },
                exec_);

}


void Foam::dummyMemoryPool::memCopy
(
    void* tgtPtr,
    void* srcPtr,
    uint64_t nElementsInBytes,
    uint64_t tgtOffsetInBytes,
    uint64_t srcOffsetInBytes
)
{
    if (!this->isValid(tgtPtr))
        return;
    blockList::iterator tgtElement = this->usedBlockList_.find(reinterpret_cast<char*>(tgtPtr));

    if (!this->isValid(srcPtr))
        return;
    blockList::iterator srcElement = this->usedBlockList_.find(reinterpret_cast<char*>(srcPtr));

    srcPtr = (char*)srcPtr + srcOffsetInBytes;
    tgtPtr = (char*)tgtPtr + tgtOffsetInBytes;
    uint64_t srcSizeInBytes = srcElement->second - srcOffsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= srcSizeInBytes)
        srcSizeInBytes = nElementsInBytes;

    const uint64_t tgtSizeInBytes = tgtElement->second - tgtOffsetInBytes;
    if (tgtSizeInBytes < srcSizeInBytes)
    {
        FatalErrorInFunction
            << " Not enough space in target block " << nl;
        return;
    }

    std::visit([this, tgtPtr, srcPtr, srcSizeInBytes](const auto& exec)
               { exec.memCopy(tgtPtr, srcPtr, srcSizeInBytes, memCopyKind::memCopyDeviceToDevice); },
               exec_);

}

void Foam::dummyMemoryPool::showAllocated(bool relative)
{
    for ( blockList::iterator ii = this->usedBlockList_.begin(); ii != this->usedBlockList_.end(); ii++ )
        Info
            << "At address: "<< reinterpret_cast<uint64_t>(ii->first)
            << " allocated block of size " << ii->second << " bytes." << nl;

}

void Foam::dummyMemoryPool::showUnallocated(bool relative)
{
    for ( blockList::iterator ii = this->unusedBlockList_.begin(); ii != this->unusedBlockList_.end(); ii++ )
        Info
            << "At address: "<< reinterpret_cast<uint64_t>(ii->first)
            << " allocated block of size " << ii->second << " bytes." << nl;

};

// read memory pool properties dictionary
void Foam::dummyMemoryPool::readProperties(const dictionary &typeDict)
{};
