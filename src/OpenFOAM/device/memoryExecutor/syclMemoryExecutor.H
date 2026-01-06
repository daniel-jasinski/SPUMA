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
    Foam::syclMemoryExecutor

Description
    SYCL executor for memory pool operations using AdaptiveCpp.

SourceFiles
    syclMemoryExecutor.H
    syclMemoryExecutor.cpp

\*---------------------------------------------------------------------------*/

#ifndef Foam_sycl_Memory_Executor_H
#define Foam_sycl_Memory_Executor_H

#include "memoryExecutor.H"
#include <sycl/sycl.hpp>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declaration
sycl::queue& getSyclQueue();

/*---------------------------------------------------------------------------*\
                       Class syclMemoryExecutor Declaration
\*---------------------------------------------------------------------------*/

class syclMemoryExecutor
: public memoryExecutor<syclMemoryExecutor>
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
        const size_t sizeInBytes,
        const void* value,
        size_t sizeOfValue
    );

    static void _backendMemSetScalarOne
    (
       void* ptr,
       const size_t sizeInBytes
    );

    static void _backendMemSet
    (
        void* ptr,
        const size_t sizeInBytes,
        const int value
    );
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "syclMemoryExecutor.cpp"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
