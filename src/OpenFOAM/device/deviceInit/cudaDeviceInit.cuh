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
    Foam::cudaDeviceInit

Description
    Cuda device initialization backend.

SourceFiles
    cudaDeviceinit.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_cuda_deviceInit_H
#define Foam_cuda_deviceInit_H

#include "deviceInit.H"
#include "label.H"
#include "IPstream.H"
#include "OPstream.H"
#ifdef have_cuda
    #include <cuda.h>
    #include <cuda_runtime_api.h>
    #include "cudaError.cuh"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class cudaDeviceInit Declaration
\*---------------------------------------------------------------------------*/

class cudaDeviceInit
: public deviceInit<cudaDeviceInit>
{
    static int devID_;

    // max shared memory per block
    static int sharedMemPerBlock_;

    // percentage of max shared memory per block usable
    static double sharedMemPerBlockP_;

public:

    static void _backendInit()
    {
        if (!initDeviceFlag_)
        {
            Info << "Initializing CUDA devices..." << nl << nl;

            label nDevs;
            cudaGetDeviceCount(&nDevs);

            devID_ = Pstream::myProcNo() % nDevs;
            cudaSetDevice(devID_);

            initDeviceFlag_ = true;
        }
    }

    static void setSharedMemP(const scalar p)
    {
        sharedMemPerBlockP_ = p;
    };

    static int getSharedMemPerBlock()
    {
        if(sharedMemPerBlock_ == -1)
        {
            // get device properties
            cudaDeviceProp prop;
            CHECK_CUDA_ERROR(
                cudaGetDeviceProperties(&prop,devID_)
            );

            sharedMemPerBlock_ = prop.sharedMemPerBlockOptin;
        }

        // note use a percentage of max allowable dynamic shared memory 
        // because driver always reserve some of the total shared memory for static allocated object
        return sharedMemPerBlock_ * sharedMemPerBlockP_;
    }

};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
