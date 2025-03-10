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

\*---------------------------------------------------------------------------*/

#include <cstring>
#include "memCopyKind.H"
#include "umpireMemoryPool.H"
#include "error.H"

namespace Foam
{
    defineTypeNameAndDebug(umpireMemoryPool,  0);
}

// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

Foam::umpireMemoryPool::umpireMemoryPool(const uint64_t size):
    Foam::MemoryPool::MemoryPool(),
    rm_(umpire::ResourceManager::getInstance()),
    inspector_()
{
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

    tmpAllocator_ = rm_.makeAllocator<umpire::strategy::DynamicPoolList>
    (
        "tmp_dynamic_pool",
        hostAllocator
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
        Info << "WARNING: trying to allocate a block of zero size." << nl;
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
    if (!this->isValid(ptr))
        return;

    allocator_.deallocate(ptr);
};

uint64_t Foam::umpireMemoryPool::arraySizeInBytes(void* poolPtr)
{
    // check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return 0;

    return allocator_.getSize(poolPtr);
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
    if (!this->isValid(poolPtr))
    {
        raisePoolValidError(poolPtr)
    }

    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    uint64_t size = allocator_.getSize(poolPtr);
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
    if (!this->isValid(poolPtr))
    {
        raisePoolValidError(poolPtr)
    }

    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    uint64_t size = allocator_.getSize(poolPtr);
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
    // check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    uint64_t size = allocator_.getSize(poolPtr);
    if (nElementsInBytes > size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // workaround to delete type info in function
    char* tmpPtr = (char*)tmpAllocator_.allocate(size);
    char* tmpValue = (char*)value;
    for (size_t i = 0; i < size; i+=sizeOfValue)
    {
        for (size_t j = 0; j < sizeOfValue; j++ )
        {
            tmpPtr[i+j] = tmpValue[j];
        }
    }

    rm_.copy(poolPtr, (void*)tmpPtr, size);
    tmpAllocator_.deallocate(tmpPtr);
};

void Foam::umpireMemoryPool::memSetScalarOne
(
    void* poolPtr,
    uint64_t nElementsInBytes
)
{
    // check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    // if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    uint64_t size = allocator_.getSize(poolPtr);
    if (nElementsInBytes > size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    scalar* tmpPtr = (scalar*)tmpAllocator_.allocate(size);
    const scalar one = 1.0;
    for (size_t i = 0; i < size/sizeof(scalar); i++)
    {
        tmpPtr[i] = one;
    }

    rm_.copy(poolPtr, (void*)tmpPtr, size);
    tmpAllocator_.deallocate(tmpPtr);
};

void Foam::umpireMemoryPool::memSet
(
    void* poolPtr,
    const int value,
    uint64_t nElementsInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    //if nElementsInBytes = 0 do nothing
    if (nElementsInBytes == 0) return;

    uint64_t size = allocator_.getSize(poolPtr);
    if (nElementsInBytes > size)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in block"
            <<abort(FatalError);
    }

    // does not work with anything but int
    rm_.memset(poolPtr,value,nElementsInBytes);
};

void Foam::umpireMemoryPool::memCopy
(
    void* tgtPtr,
    void* srcPtr,
    uint64_t nElementsInBytes
)
{
    // check if allocation record associated with an tgtPtr and srcPtr exist
    // if ptr is null do nothing
    if (tgtPtr == nullptr) return;

    if (nElementsInBytes == 0) return;

    if(!this->isValid(tgtPtr) || !this->isValid(srcPtr))
        raisePoolValidError(poolPtr);

    uint64_t srcSizeInBytes = this->allocator_.getSize(srcPtr);
    const uint64_t tgtSizeInBytes = this->allocator_.getSize(tgtPtr);

    if (nElementsInBytes > srcSizeInBytes)
    {
        FatalErrorInFunction
            << "Trying to read more bytes than available in src block"
            <<abort(FatalError);
    }

    if (nElementsInBytes > tgtSizeInBytes)
    {
        FatalErrorInFunction
            << "Trying to assign more bytes than available in target block"
            <<abort(FatalError);
    }

    rm_.copy(tgtPtr,srcPtr,nElementsInBytes);
}

// ************************************************************************* //
