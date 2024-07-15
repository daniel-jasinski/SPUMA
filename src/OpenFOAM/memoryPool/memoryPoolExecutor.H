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

Class
    memoryPoolExecutor

Description
    List of memoryPoolExecutors used as Visitors in the fixedSizeMemoryPool class

SourceFiles
    MemoryPoolExecutor.H
*/


#ifndef memory_Pool_Executor_H
#define memory_Pool_Executor_H

#ifdef have_cuda
    #ifdef have_managed
        #include "cudaManagedMemoryPoolExecutor.H"
    #else
        #include "cudaMemoryPoolExecutor.H"
    #endif
#else
#include "cpuMemoryPoolExecutor.H"
#endif
#include <variant>

namespace Foam
{

using memoryPoolExecutor = std::variant
    <
#ifdef have_cuda
    #ifdef have_managed
    cudaManagedMemoryPoolExecutor
    #else
    cudaMemoryPoolExecutor
    #endif
#else
    cpuMemoryPoolExecutor
#endif
    >;


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
