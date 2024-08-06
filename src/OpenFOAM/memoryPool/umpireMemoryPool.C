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
#include "memCopyKind.H"
#include "umpireMemoryPool.H"
#include "error.H"
#include "dictionary.H"

namespace Foam
{
    defineTypeNameAndDebug(umpireMemoryPool,  0);
    addToRunTimeSelectionTable(MemoryPool, umpireMemoryPool, dictionary);
    addToRunTimeSelectionTable(MemoryPool, umpireMemoryPool, word);
}
// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //

Foam::umpireMemoryPool::umpireMemoryPool(const uint64_t size):
    Foam::MemoryPool::MemoryPool(),
    rm_(umpire::ResourceManager::getInstance()),
    inspector_()
{
#ifdef have_cuda
        #ifdef have_managed
        auto allocator = rm_.getAllocator("UM");
        #else
        auto allocator = rm_.getAllocator("DEVICE");
        #endif
#else
        auto allocator = rm_.getAllocator("HOST");
#endif

    auto hostAllocator = rm_.getAllocator("HOST");

    constexpr uint64_t GB = 1024ul*1024*1024;
    initialSize_ = size >  GB ? size : GB;
    minBlockSize_ = 1024*1024; //1Mb

    allocator_ = rm_.makeAllocator<umpire::strategy::DynamicPoolList>("dynamic_pool",allocator,
                                                                        initialSize_, /*default 512 Mb*/
                                                                        minBlockSize_); /*default 1Mb*/

    tmpAllocator_ = rm_.makeAllocator<umpire::strategy::DynamicPoolList>("tmp_dynamic_pool", hostAllocator);
    st_ = new umpire::strategy::DynamicPoolList("strategy",hostAllocator.getId(),hostAllocator);

};

Foam::umpireMemoryPool::umpireMemoryPool(const dictionary& dict):
    Foam::MemoryPool::MemoryPool(dict),
    rm_(umpire::ResourceManager::getInstance()),
    inspector_()
{
#ifdef have_cuda
        #ifdef have_managed
        auto allocator = rm_.getAllocator("UM");
        #else
        auto allocator = rm_.getAllocator("DEVICE");
        #endif
#else
        auto allocator = rm_.getAllocator("HOST");
#endif

    this->readProperties(dict.subDict(this->type()));

    auto hostAllocator = rm_.getAllocator("HOST");
    allocator_ = rm_.makeAllocator<umpire::strategy::DynamicPoolList>("dynamic_pool",allocator,
                                                                        initialSize_, /*default 512 Mb*/
                                                                        minBlockSize_); /*default 1Mb*/

    tmpAllocator_ = rm_.makeAllocator<umpire::strategy::DynamicPoolList>("tmp_dynamic_pool", hostAllocator);
    st_ = new umpire::strategy::DynamicPoolList("strategy",hostAllocator.getId(),hostAllocator);

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

// free function
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
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return 0;
    return allocator_.getSize(poolPtr);
};

void Foam::umpireMemoryPool::copyIn(void* poolPtr, void* ptr, uint64_t nElementsInBytes, uint64_t offsetInBytes)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;
    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    uint64_t sizeInBytes = allocator_.getSize(poolPtr);
    poolPtr =  (char*) poolPtr + offsetInBytes;
    sizeInBytes -= offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= sizeInBytes)
        sizeInBytes = nElementsInBytes;

    // workaround: umpire does not support copies between non umpire pointers
    inspector_.registerAllocation(ptr, sizeInBytes, st_);
    rm_.copy(poolPtr, ptr, sizeInBytes);
    inspector_.deregisterAllocation(ptr, st_);
};

void Foam::umpireMemoryPool::copyOut(void* poolPtr, void* ptr, uint64_t nElementsInBytes, uint64_t offsetInBytes)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;
    if (!ptr)
        FatalErrorInFunction << "source pointer is null" << abort(FatalError);

    uint64_t sizeInBytes = allocator_.getSize(poolPtr);
    poolPtr = (char*)poolPtr + offsetInBytes;
    sizeInBytes -= offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= sizeInBytes)
        sizeInBytes = nElementsInBytes;

    inspector_.registerAllocation(ptr, sizeInBytes, st_);
    rm_.copy(ptr, poolPtr, sizeInBytes);
    inspector_.deregisterAllocation(ptr, st_);
};

void Foam::umpireMemoryPool::memSet
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

    uint64_t size = allocator_.getSize(poolPtr) - offsetInBytes;
    poolPtr = (char*)poolPtr + offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(size))
        size = nElementsInBytes;
    /*
    T* tmpPtr =(T*)tmpAllocator_.allocate(sizeof(T));
    *tmPtr= value;
    this->rm_.copy(poolPtr,tmpPtr,sizeof(T));
    trova modo di copiare il primo elemento del pool sul resto
    this->tmpAllocator_.deallocate(tmpPtr);
    */

    //-- workaround to delete type info in function --//
    char* tmpPtr = (char*)tmpAllocator_.allocate(size);
    char* tmpValue = (char*)value;
    for (size_t i = 0; i < size; i+=sizeOfValue)
    {
        for (size_t j = 0; j < sizeOfValue; j++ )
        {
            tmpPtr[i+j] = tmpValue[j];
        }
    }
    // -------------------------------------------//
    rm_.copy(poolPtr, (void*)tmpPtr, size);
    tmpAllocator_.deallocate(tmpPtr);
};

void Foam::umpireMemoryPool::memSetScalarOne
(
    void* poolPtr,
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    uint64_t size = allocator_.getSize(poolPtr) - offsetInBytes;
    poolPtr = (char*)poolPtr + offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(size))
        size = nElementsInBytes;
    /*
    T* tmpPtr =(T*)tmpAllocator_.allocate(sizeof(T));
    *tmPtr= value;
    this->rm_.copy(poolPtr,tmpPtr,sizeof(T));
    trova modo di copiare il primo elemento del pool sul resto
    this->tmpAllocator_.deallocate(tmpPtr);
    */
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
    uint64_t nElementsInBytes,
    uint64_t offsetInBytes
)
{
    //check if pointer was allocated with pool
    if (!this->isValid(poolPtr))
        return;

    uint64_t size = allocator_.getSize(poolPtr) - offsetInBytes;
    poolPtr = (char*)poolPtr + offsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= static_cast<uint64_t>(size))
        size = nElementsInBytes;

    rm_.memset(poolPtr,value,size); // does not work with anything but int
};

void Foam::umpireMemoryPool::memCopy(
    void* tgtPtr,
    void* srcPtr,
    uint64_t nElementsInBytes,
    uint64_t tgtOffsetInBytes,
    uint64_t srcOffsetInBytes
)
{
    //check if allocation record associated with an tgtPtr and srcPtr exist
    if(!this->isValid(tgtPtr) && !this->isValid(srcPtr))
        return;

    uint64_t srcSizeInBytes = this->allocator_.getSize(srcPtr) - srcOffsetInBytes;
    const uint64_t tgtSizeInBytes = this->allocator_.getSize(tgtPtr) -  tgtOffsetInBytes;
    srcPtr = (char*)srcPtr + srcOffsetInBytes;
    tgtPtr = (char*)tgtPtr + tgtOffsetInBytes;
    if (nElementsInBytes != 0 && nElementsInBytes <= srcSizeInBytes)
        srcSizeInBytes = nElementsInBytes;

    if (tgtSizeInBytes < srcSizeInBytes)
    {
        FatalErrorInFunction
            <<" Not enough space in target block "<< nl;
        return;
    }

    rm_.copy(tgtPtr,srcPtr,srcSizeInBytes);
}

void Foam::umpireMemoryPool::readProperties(const dictionary& typeDict)
{
    constexpr uint64_t MB = 1024*1024ul;
    initialSize_ = typeDict.get<uint64_t>("initialSize")*MB;
    minBlockSize_ = typeDict.get<uint64_t>("minBlockSize")*MB;
}
