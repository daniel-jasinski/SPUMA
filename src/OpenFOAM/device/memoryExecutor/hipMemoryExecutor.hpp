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
    Foam::hipMemoryExecutor

Description
    Hip executor for the memory pool operations.

SourceFiles
    hipMemoryExecutor.hpp
    hipMemoryExecutor.hip

\*---------------------------------------------------------------------------*/

#ifndef Foam_hip_Memory_Executor_H
#define Foam_hip_Memory_Executor_H

#include "memoryExecutor.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class hipMemoryExecutor Declaration
\*---------------------------------------------------------------------------*/

class hipMemoryExecutor
: public memoryExecutor<hipMemoryExecutor>
{
public:


    static constexpr label _memAlignBytes = 16;

    static void* _backendAlloc(uint64_t size);

    static void _backendClear(void* ptr);

    static void _backendMemCopy
    (
        void* dst,
        const void* src,
        uint64_t size,
        memCopyKind kind
    );

    static void _backendMemSet
    (
        void* ptr,
        const uint64_t sizeInBytes,
        const void* value,
        size_t sizeOfValue
    );

    static void _backendMemSetScalarOne
    (
        void* ptr,
        const uint64_t sizeInBytes
    );

    static void _backendMemSet
    (
        void* ptr,
        const uint64_t sizeInBytes,
        const int value
    );
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif
