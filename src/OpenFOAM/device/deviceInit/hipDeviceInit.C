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

#ifndef Foam_hipDeviceInit_C
#define Foam_hipDeviceInit_C

#include "hipDeviceInit.hpp"
#include "IPstream.H"
#include "OPstream.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

int hipDeviceInit::devID_ = -1;
int hipDeviceInit::nThreadsPerBlock_ = 128;
double hipDeviceInit::sharedMemoryPerBlockPercentage_ = 0.9;
hipDeviceProp_t hipDeviceInit::prop_{};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

void Foam::hipDeviceInit::_backendInit()
{
    if (initDeviceFlag_)  return;

    Info << "Initializing HIP devices" << nl << nl;

    label driverVersion = 0;
    label runtimeVersion = 0;
    hipDriverGetVersion(&driverVersion);
    hipRuntimeGetVersion(&runtimeVersion);

    const label majorDriverVersion = driverVersion/1000;
    const label minorDriverVersion = (driverVersion % 100) / 10;
    const label majorRuntimeVersion = runtimeVersion/1000;
    const label minorRuntimeVersion = (runtimeVersion % 100) / 10;

    Info << "HIP Driver Version / Runtime Version:  "
        << majorDriverVersion << "." << minorDriverVersion << " / "
        << majorRuntimeVersion << "." << minorRuntimeVersion << nl;

    label nDevs;
    hipGetDeviceCount(&nDevs);

    devID_ = Pstream::myProcNo() % nDevs;
    
    label managedMemory = 0;
    CHECK_HIP_ERROR
    (
        hipDeviceGetAttribute
        (
            &managedMemory,
            hipDeviceAttributeManagedMemory,
            devID_
        )
    );

    if (!managedMemory)
    {
        FatalErrorInFunction
            << "managed memory access not supported "
            << "on device " << devID_
            << exit(FatalError);
    }
    
    hipSetDevice(devID_);

    CHECK_HIP_ERROR
    (
        hipGetDeviceProperties(&prop_,devID_)
    );

    initDeviceFlag_ = true;
}

void Foam::hipDeviceInit::_setNumberOfThreadsPerBlock
(
    const int nThreadsPerBlock
)
{
    if (nThreadsPerBlock > prop_.maxThreadsPerBlock)
    {
        FatalErrorInFunction
            << "trying to set a number of thread per block greater than max, "
            << "max number of thread per block is: "
            << prop_.maxThreadsPerBlock << abort(FatalError);
    }
    else if (nThreadsPerBlock <= 0)
    {
        FatalErrorInFunction
            << "trying to set a number of threads less or equal to zero"
            << abort(FatalError);
    }

    nThreadsPerBlock_ = nThreadsPerBlock;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
