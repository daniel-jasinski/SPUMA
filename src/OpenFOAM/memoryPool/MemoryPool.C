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

#include "MemoryPool.H"
#include "error.H"
#ifdef have_umpire
    #include "umpireMemoryPool.H"
#endif
#include "fixedSizeMemoryPool.H"
#include "dummyMemoryPool.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(MemoryPool,  0);
}

// Null, because instance will be initialized on demand.
Foam::MemoryPool* Foam::MemoryPool::instance = nullptr;

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::MemoryPool::MemoryPool(const dictionary& dict):
    size_(0),
    allocatedSize_(0),
    unallocatedSize_(0),
    maxOccupancy_(0)
{};

// * * * * * * * * * * * * * * * * Destructors  * * * * * * * * * * * * * * //

Foam::MemoryPool::~MemoryPool()
{
    instance = nullptr;
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::MemoryPool* Foam::MemoryPool::New
(
    const word& type,
    const uint64_t size
)
{
    // If a dummyMemoryPool was auto-created during DLL static init
    // (by getInstance()), replace it with the requested pool type.
    if (instance && type != "dummyMemoryPool"
        && dynamic_cast<dummyMemoryPool*>(instance))
    {
        // Live allocations become stranded: their pointers are unknown
        // to the replacement pool, so a later free() of one fails with a
        // pool-validity error. Warn here so that error is explicable.
        // Allocations that live for the whole run are unaffected.
        if (instance->allocatedSize() != 0)
        {
            WarningInFunction
                << "Replacing the auto-created dummyMemoryPool while it"
                << " still holds " << instance->allocatedSize()
                << " bytes of live allocations" << nl;
        }
        delete instance;
        instance = nullptr;
    }

    if (!instance)
    {
        if(type == "fixedSizeMemoryPool")
        {
            instance = new fixedSizeMemoryPool(size);
        }
        else if (type == "dummyMemoryPool")
        {
            instance = new dummyMemoryPool(size);
        }
#ifdef have_umpire
        else if (type == "umpireMemoryPool")
        {
            instance = new umpireMemoryPool(size);
        }
#endif
        else
        {
            FatalErrorInFunction
            << type << " does not exist. "
            << "Please use a different memory pool." << nl
            << abort(FatalError);
        }
    }

    return instance;
}

Foam::MemoryPool* Foam::MemoryPool::getInstance()
{
    if (!instance)
    {
        // Auto-create a dummyMemoryPool when getInstance() is called
        // before explicit New(). This happens during DLL static init
        // on Windows when some List operations need pool access.
        instance = new dummyMemoryPool(0);
    }
    return instance;
}

// ************************************************************************* //
