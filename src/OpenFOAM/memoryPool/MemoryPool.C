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

#include "MemoryPool.H"
#include "error.H"
#include "dictionary.H"
#ifdef have_umpire
    #include "umpireMemoryPool.H"
#endif
#include "fixedSizeMemoryPool.H"
#include "dummyMemoryPool.H"

namespace Foam
{
    defineTypeNameAndDebug(MemoryPool,  0);
    defineRunTimeSelectionTable(MemoryPool,dictionary);
    defineRunTimeSelectionTable(MemoryPool,word);
}
/* Null, because instance will be initialized on demand. */
Foam::MemoryPool* Foam::MemoryPool::instance = nullptr;

// Constructors
Foam::MemoryPool::MemoryPool(const dictionary& dict):
    size_(0),
    allocatedSize_(0),
    unallocatedSize_(0),
    maxOccupancy_(0)
{
    //this->readProperties(dict);
};

// Selector
Foam::MemoryPool* Foam::MemoryPool::New
(
    const dictionary& dict
)
{
    if (!instance)
    {
        const word poolType(dict.get<word>("type"));

        auto* ctroPtr = dictionaryConstructorTable(poolType);

        if (!ctroPtr)
        {
            FatalIOErrorInFunction(dict)
                << "Unknown memory pool type " << poolType << nl
                << "Valid memory pool types are : "<< nl << nl
                << dictionaryConstructorTablePtr_->sortedToc()
                << exit(FatalIOError);
        };

        instance = autoPtr<MemoryPool>(ctroPtr(dict)).release();

        const bool printProperties(dict.getOrDefault("printProperties", 1));
        /*if (printProperties)
        {
            Info
                << "created memory pool of type : "
                << instance->type() << nl
                << "memoryResource : " << instance->memoryResource_ << nl << nl
                << instance->type()
                << instance->properties_ << endl;
        }*/
    }

    return instance;
}

Foam::MemoryPool* Foam::MemoryPool::New
(
    const word& type,
    const uint64_t size
)
{
    if (!instance)
    {
        auto* ctroPtr = wordConstructorTable(type);
        if (!ctroPtr)
        {
            FatalErrorInFunction
                << "Unknown memory pool type " << type << nl
                << "Valid memory pool types are : "<< nl << nl
                << wordConstructorTablePtr_->sortedToc()
                << exit(FatalError);
        };

        instance = autoPtr<MemoryPool>(ctroPtr(size)).release();
    }

    return instance;
}



Foam::MemoryPool* Foam::MemoryPool::getInstance()
{
    if (!instance)
    {
       FatalErrorInFunction
        << "no instance of memory pool initialized" << nl
        << abort(FatalError);
    }

    return instance;
}
