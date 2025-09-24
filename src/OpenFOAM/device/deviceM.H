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

Description
    Collection of device MACROs

SourceFiles
    deviceM.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_device_M_H
#define Foam_device_M_H

#if defined(have_cuda) || defined(have_hip)
    #define FOAM_DEVICE __host__ __device__
    #define FOAM_DEVICE_I __host__ __device__ __forceinline__
#else
    #define FOAM_DEVICE
    #define FOAM_DEVICE_I inline
#endif

// utility
#ifdef have_cuda
    #include <cuda_runtime_api.h>
    #define deviceSync() cudaDeviceSynchronize()
#elif have_hip
    #include <hip/hip_runtime.h>
    #define deviceSync() hipDeviceSynchronize()
#else
    #define deviceSync()
#endif


// Macro for loops over cells with ldu addressing
#define forAllOwner(ownStart,idCell,idFace,expression) \
    for (size_t idFace = ownStart[idCell]; idFace < ownStart[idCell+1]; idFace++)\
    {\
        expression\
    };
#define forAllNbr(losortStart,losort,idCell,nbrFace,expression)\
    for (size_t idlosort = losortStart[idCell]; idlosort < losortStart[idCell+1]; idlosort++)\
    {\
        const Foam::label nbrFace = losort[idlosort];\
        if(nbrFace != -1){\
            expression\
        };\
    };

#define parallelForAll(size,index,expression)\
    Foam::foamExecutor exec;\
    auto Lambda = [=](label index){expression};\
    exec.parallelFor(Lambda,size);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
