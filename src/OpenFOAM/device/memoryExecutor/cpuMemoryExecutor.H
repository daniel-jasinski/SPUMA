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

Class
    Foam::cpuMemoryExecutor

Description
    CPU executor for the memory pool operations.

SourceFiles
    cpuMemoryExecutor.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_cpu_Memory_Executor_H
#define Foam_cpu_Memory_Executor_H

#include "memoryExecutor.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class cpuMemoryExecutor Declaration
\*---------------------------------------------------------------------------*/

class cpuMemoryExecutor
: public memoryExecutor<cpuMemoryExecutor>
{
public:

    //- static data

    static constexpr label _memAlignBytes = 8;

    //- static member functions

    static void* _backendAlloc(uint64_t size)
    {
        return new char[size];
    }

    static void _backendClear(void* ptr)
    {
        delete [] static_cast<char*>(ptr);
    }

    static void _backendMemCopy
    (
        void* dst,
        const void* src,
        uint64_t size,
        memCopyKind kind
    )
    {
        std::memcpy(dst, src, static_cast<size_t>(size));
    }

    static void _backendMemSet
    (
        void* ptr,
        const uint64_t sizeInBytes,
        const void* value, size_t sizeOfValue
    )
    {
        char* tmpPtr = static_cast<char*>(ptr);
        const char* tmpValue = static_cast<const char*>(value);
        for (size_t i = 0; i < sizeInBytes; i+=sizeOfValue)
        {
            for (size_t j = 0; j < sizeOfValue; j++ )
            {
                tmpPtr[i+j] = tmpValue[j];
            }
        }
    }

    static void _backendMemSetScalarOne(void* ptr, const size_t sizeInBytes)
    {
        scalar* tmpPtr = static_cast<scalar*>(ptr);
        for (size_t i = 0; i < sizeInBytes/sizeof(scalar); i++)
        {
            tmpPtr[i] = 1.0;
        }
    }

    static void _backendMemSet
    (
        void* ptr,
        const uint64_t sizeInBytes,
        const int value
    )
    {
        std::memset(ptr, value, sizeInBytes);
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
