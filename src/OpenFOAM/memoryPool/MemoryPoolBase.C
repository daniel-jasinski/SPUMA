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

#include "MemoryPoolBase.H"
#include "error.H"
#ifdef have_umpire
    #include "umpireMemoryPool.H"
#endif
#include "fixedSizeMemoryPool.H"
#include "dummyMemoryPool.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace Spuma
{
    defineTypeNameAndDebug(MemoryPool,  0);
}

}

// Null, because instance will be initialized on demand.
Foam::Spuma::MemoryPool* Foam::Spuma::MemoryPool::instance = nullptr;

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::Spuma::MemoryPool::MemoryPool(const dictionary& dict):
    size_(0),
    allocatedSize_(0),
    unallocatedSize_(0),
    maxOccupancy_(0)
{};

// * * * * * * * * * * * * * * * * Destructors  * * * * * * * * * * * * * * //

Foam::Spuma::MemoryPool::~MemoryPool()
{
    delete instance;
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::Spuma::MemoryPool* Foam::Spuma::MemoryPool::New
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

Foam::Spuma::MemoryPool* Foam::Spuma::MemoryPool::getInstance()
{
    if (!instance)
    {
       FatalErrorInFunction
        << "no instance of memory pool initialized" << nl
        << abort(FatalError);
    }

    return instance;
}

// ************************************************************************* //
