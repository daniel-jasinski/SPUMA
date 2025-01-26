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
#ifdef have_umpire
    #include "umpireMemoryPool.H"
#endif
#include "fixedSizeMemoryPool.H"
#include "dummyMemoryPool.H"

namespace Foam
{
    defineTypeNameAndDebug(MemoryPool,  0);
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

// Destructors
Foam::MemoryPool::~MemoryPool()
{
    delete instance;
}
// Selector

Foam::MemoryPool* Foam::MemoryPool::New
(
    const word& type,
    const uint64_t size
)
{
    if (!instance)
    {
        if(type == "fixedSizeMemoryPool")
        {
            instance = new fixedSizeMemoryPool(size);
        }
        else if (type == "dummyMemoryPool")
        {
            instance = new  dummyMemoryPool(size);
        }
        else
        {
            FatalErrorInFunction
            << "wrong memory pool type" << nl
            << abort(FatalError);
        }
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
