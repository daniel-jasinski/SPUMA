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
    Foam::hipDeviceInit

Description
    Hip (AMD ROCm) device initialization backend

SourceFiles
    hipDeviceInit.hpp

\*---------------------------------------------------------------------------*/

#ifndef Foam_hip_deviceInit_H
#define Foam_hip_deviceInit_H

#include "deviceInit.H"
#include "label.H"
#include "IPstream.H"
#include "OPstream.H"
#ifdef have_hip
    #include <hip/hip_runtime.h>
    #include "hipError.hpp"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                          Class hipDeviceInit Declaration
\*---------------------------------------------------------------------------*/

class hipDeviceInit
: public deviceInit<hipDeviceInit>
{
    static int devID_;

    // percentage of max shared memory per block usable
    static double sharedMemPerBlockP_;

    // number of thread per block
    static int threadBlock_;

    // hip property struct
    static hipDeviceProp_t prop_;

public:

    static void _backendInit();

    static void setSharedMemP(const scalar p)
    {
        sharedMemPerBlockP_ = p;
    };
    
    static int getSharedMemPerBlock()
    {
        // note use a percentage of max allowable dynamic shared memory 
        // because driver always reserve some of the total shared memory for static allocated object
        return prop_.sharedMemPerBlockOptin * sharedMemPerBlockP_;
    }

    static int getSM()
    {
        return prop_.multiProcessorCount;
    }

    static void _setNThreads(const int tBlock);

    static int getThreadsPerBlock()
    {
        return threadBlock_;
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
