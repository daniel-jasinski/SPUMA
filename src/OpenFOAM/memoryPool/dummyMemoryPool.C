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
#include <cstdlib>
#include "dummyMemoryPool.H"
#include "error.H"

namespace Foam
{
    defineTypeNameAndDebug(dummyMemoryPool, 0);
}

// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

Foam::dummyMemoryPool::dummyMemoryPool(const uint64_t size):
    Foam::MemoryPool::MemoryPool()
{};

// * * * * * * * * * * * * * * * Destructors  * * * * * * * * * * * * * * * //

Foam::dummyMemoryPool::~dummyMemoryPool()
{
    DebugInFunction << "MEMPOOL: destroy dummy memory Pool" << nl;
};

// * * * * * * * * * * * * * Public Member Functions  * * * * * * * * * * * //

void* Foam::dummyMemoryPool::allocate(uint64_t size)
{
    if (!size)
    {
        WarningInFunction<< "Trying to allocate a block of zero size." << nl;
        return nullptr;
    }
    void* head = std::malloc(size);

    usedBlockList_.insert(blockPair(static_cast<char*>(head), size));
    this->size_ += size;
    this->allocatedSize_ += size;

    this->maxOccupancy_ =
        size > this->maxOccupancy_ ? size : this->maxOccupancy_;

    return head;
};

// free function
void Foam::dummyMemoryPool::free(void* ptr)
{
    //if ptr is null do nothing
    if (ptr == nullptr) return;

    //check if pointer was allocated with pool
    if (!this->isValid(ptr))
    {
        raisePoolValidError(ptr)
    }

    blockList::iterator block = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(ptr)
    );

    std::free(ptr);

    uint64_t size = block->second;
    this->unusedBlockList_.insert(blockPair(block->first, size));

    DebugInFunction
        << "Deallocated block of size " << size
        << " at address " << std::to_string(reinterpret_cast<uint64_t>(ptr)) << nl;

    this->allocatedSize_   -= size;
    this->size_            -= size;
    this->unallocatedSize_ += size;

    this->usedBlockList_.erase(reinterpret_cast<char*>(ptr));
};

// Returns size of array (in bytes if type not specified)
uint64_t Foam::dummyMemoryPool::arraySizeInBytes(void* poolPtr)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return 0;

    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
    {
        raisePoolValidError(poolPtr)
    }

    blockList::iterator mapElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(poolPtr)
    );

    return mapElement->second;
};

void Foam::dummyMemoryPool::copyIn
(
    void* poolPtr,
    void* ptr,
    uint64_t nElementsInBytes
)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return;

    //if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
    {
        raisePoolValidError(poolPtr)
    }

    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    blockList::iterator mapElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(poolPtr)
    );

    if (nElementsInBytes > mapElement->second)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    std::memcpy(poolPtr, ptr, nElementsInBytes);
};

void Foam::dummyMemoryPool::copyOut
(
    void* poolPtr,
    void* ptr,
    uint64_t nElementsInBytes
)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return;

    //if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
    {
        raisePoolValidError(poolPtr)
    }

    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    blockList::iterator mapElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(poolPtr)
    );

    if (nElementsInBytes > mapElement->second)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    std::memcpy(ptr, poolPtr, nElementsInBytes);
};

void Foam::dummyMemoryPool::memSet
(
    void* poolPtr,
    const void* value,
    size_t sizeOfValue,
    uint64_t nElementsInBytes
)
{
    //if ptr is null do nothing
    if (poolPtr == nullptr) return;

    //if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    void* allocatedPoolPtr = poolPtr;

    if (!this->isValid(poolPtr))
    {
        //find nearest valid pointer
        if(!this->isInBlockRange(poolPtr))
        {
            FatalErrorInFunction
                << "MEMPOOL: src pointer " << std::to_string(reinterpret_cast<uint64_t>(poolPtr))
                << " is not valid and not in range" << abort(FatalError);
        }
        uint64_t ptr = reinterpret_cast<uint64_t>(poolPtr);

        for (auto block = this->usedBlockList_.rbegin();
                  block!= this->usedBlockList_.rend(); ++block)
        {
            uint64_t allocatedPtr = reinterpret_cast<uint64_t>(block->first);
            if ( (ptr>allocatedPtr) && (ptr < allocatedPtr + block->second) )
            {
                allocatedPoolPtr = reinterpret_cast<void*>(allocatedPtr);
                break;
            }
        }
    }
    blockList::iterator mapElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(allocatedPoolPtr)
    );

    if (reinterpret_cast<uint64_t>(poolPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedPoolPtr) + mapElement->second)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // Replicate the value pattern across the memory block
    char* dst = static_cast<char*>(poolPtr);
    const char* val = static_cast<const char*>(value);
    for (uint64_t i = 0; i < nElementsInBytes; i += sizeOfValue)
    {
        std::memcpy(dst + i, val, sizeOfValue);
    }
}

void Foam::dummyMemoryPool::memSetScalarOne
(
    void* poolPtr,
    uint64_t nElementsInBytes
)
{
    // if ptr is null do nothing
    if (poolPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    void* allocatedPoolPtr = poolPtr;
    if (!this->isValid(poolPtr))
    {
        // find nearest valid pointer
        if(!this->isInBlockRange(poolPtr))
        {
            FatalErrorInFunction
                << "MEMPOOL: src pointer " << std::to_string(reinterpret_cast<uint64_t>(poolPtr))
                << " is not valid and not in range" << abort(FatalError);
        }
        uint64_t ptr = reinterpret_cast<uint64_t>(poolPtr);

        for (auto block = this->usedBlockList_.rbegin();
                  block!= this->usedBlockList_.rend(); ++block)
        {
            uint64_t allocatedPtr = reinterpret_cast<uint64_t>(block->first);
            if ((ptr>allocatedPtr) && (ptr < allocatedPtr + block->second))
            {
                allocatedPoolPtr = reinterpret_cast<void*>(allocatedPtr);
                break;
            }
        }
    }
    blockList::iterator mapElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(allocatedPoolPtr)
    );

    if (reinterpret_cast<uint64_t>(poolPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedPoolPtr) + mapElement->second)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // Set each scalar element to 1.0
    const size_t numScalars = nElementsInBytes / sizeof(Foam::scalar);
    Foam::scalar* scalarPtr = static_cast<Foam::scalar*>(poolPtr);
    for (size_t i = 0; i < numScalars; ++i)
    {
        scalarPtr[i] = Foam::scalar(1);
    }
}

void Foam::dummyMemoryPool::memSet
(
    void* poolPtr,
    const int value,
    uint64_t nElementsInBytes
)
{
    // if ptr is null do nothing
    if (poolPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    void* allocatedPoolPtr = poolPtr;

    if (!this->isValid(poolPtr))
    {
        // find nearest valid pointer
        if(!this->isInBlockRange(poolPtr))
        {
            FatalErrorInFunction
                << "MEMPOOL: src pointer " << std::to_string(reinterpret_cast<uint64_t>(poolPtr))
                << " is not valid and not in range" << abort(FatalError);
        }
        uint64_t ptr = reinterpret_cast<uint64_t>(poolPtr);

        for (auto block = this->usedBlockList_.rbegin();
                  block!= this->usedBlockList_.rend(); ++block)
        {
            uint64_t allocatedPtr = reinterpret_cast<uint64_t>(block->first);
            if ((ptr>allocatedPtr) && (ptr < allocatedPtr + block->second))
            {
                allocatedPoolPtr = reinterpret_cast<void*>(allocatedPtr);
                break;
            }
        }
    }

    blockList::iterator mapElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(allocatedPoolPtr)
    );

    if (reinterpret_cast<uint64_t>(poolPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedPoolPtr) + mapElement->second)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    std::memset(poolPtr, value, nElementsInBytes);
}

void Foam::dummyMemoryPool::memCopy
(
    void* tgtPtr,
    void* srcPtr,
    uint64_t nElementsInBytes
)
{
    // if ptr is null do nothing
    if (tgtPtr == nullptr) return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    void* allocatedTgtPtr = tgtPtr;
    if (!this->isValid(tgtPtr))
    {
        // find nearest valid pointer
        if(!this->isInBlockRange(tgtPtr))
        {
            FatalErrorInFunction
                << "MEMPOOL: src pointer " << std::to_string(reinterpret_cast<uint64_t>(tgtPtr))
                << " is not valid and not in range" << abort(FatalError);
        }
        uint64_t ptr = reinterpret_cast<uint64_t>(tgtPtr);

        for (auto block = this->usedBlockList_.rbegin();
                  block!= this->usedBlockList_.rend(); ++block)
        {
            uint64_t allocatedPtr = reinterpret_cast<uint64_t>(block->first);
            if ((ptr>allocatedPtr) && (ptr < allocatedPtr + block->second))
            {
                allocatedTgtPtr = reinterpret_cast<void*>(allocatedPtr);
                break;
            }
        }
    }

    blockList::iterator tgtElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(allocatedTgtPtr)
    );

    void* allocatedSrcPtr = srcPtr;
    if (!this->isValid(srcPtr))
    {
        if(!this->isInBlockRange(srcPtr))
        {
            FatalErrorInFunction
                << "MEMPOOL: src pointer " << std::to_string(reinterpret_cast<uint64_t>(srcPtr))
                << " is not valid and not in range" << abort(FatalError);
        }

        uint64_t ptr = reinterpret_cast<uint64_t>(srcPtr);

        //search for the reference pointer of the block
        for (auto block = this->usedBlockList_.rbegin();
                  block!= this->usedBlockList_.rend(); ++block)
        {
            uint64_t allocatedPtr = reinterpret_cast<uint64_t>(block->first);
            if ((ptr>allocatedPtr) && (ptr < allocatedPtr + block->second))
            {
                allocatedSrcPtr = reinterpret_cast<void*>(allocatedPtr);
                break;
            }
        }
    }

    blockList::iterator srcElement = this->usedBlockList_.find
    (
        reinterpret_cast<char*>(allocatedSrcPtr)
    );

    if (reinterpret_cast<uint64_t>(srcPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedSrcPtr) + srcElement->second)
    {
        FatalErrorInFunction
            << "Trying to read more bytes than available in src block"
            <<abort(FatalError);
    }

    if (reinterpret_cast<uint64_t>(tgtPtr) + nElementsInBytes >
        reinterpret_cast<uint64_t>(allocatedTgtPtr) + tgtElement->second)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in tgt block"
            <<abort(FatalError);
    }

    std::memcpy(tgtPtr, srcPtr, nElementsInBytes);
}

void Foam::dummyMemoryPool::showAllocated(bool relative)
{
    for (blockList::iterator ii = this->usedBlockList_.begin();
                             ii != this->usedBlockList_.end(); ii++)
        Info
            << "At address: " << std::to_string(reinterpret_cast<uint64_t>(ii->first))
            << " allocated block of size " << ii->second << " bytes." << nl;
}

void Foam::dummyMemoryPool::showUnallocated(bool relative)
{
    for (blockList::iterator ii = this->unusedBlockList_.begin();
                             ii != this->unusedBlockList_.end(); ii++)
        Info
            << "At address: " << std::to_string(reinterpret_cast<uint64_t>(ii->first))
            << " allocated block of size " << ii->second << " bytes." << nl;
};

// ************************************************************************* //
