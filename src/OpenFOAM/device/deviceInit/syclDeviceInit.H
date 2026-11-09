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
    Foam::syclDeviceInit

Description
    SYCL device initialization backend using AdaptiveCpp.
    Attempts GPU selection with CPU fallback.

SourceFiles
    syclDeviceInit.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_sycl_deviceInit_H
#define Foam_sycl_deviceInit_H

#include "deviceInit.H"
#include "label.H"
#include "messageStream.H"
#include "error.H"
#include <sycl/sycl.hpp>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Global SYCL queue - defined in syclGlobal.cpp
sycl::queue& getSyclQueue();
bool syclQueueActive();
bool syclQueueShutdown();
void destroySyclQueue();

/*---------------------------------------------------------------------------*\
                       Class syclDeviceInit Declaration
\*---------------------------------------------------------------------------*/

class syclDeviceInit
: public deviceInit<syclDeviceInit>
{
    static int nThreadsPerBlock_;

public:

    static void _setNumberOfThreadsPerBlock(const int nThreadsPerBlock);
    static int _getNumberOfThreadsPerBlock();

    static void _backendShutdown()
    {
        if (initDeviceFlag_)
        {
            destroySyclQueue();
            initDeviceFlag_ = false;
        }
    }

    static void _backendInit()
    {
        if (!initDeviceFlag_)
        {
            Info << "Initializing SYCL devices..." << nl << nl;

            try
            {
                // Get the global queue (will be initialized on first access)
                sycl::queue& q = getSyclQueue();

                // Print device info
                auto device = q.get_device();
                Info << "  SYCL Device: "
                     << device.get_info<sycl::info::device::name>()
                     << nl;
                Info << "  Vendor: "
                     << device.get_info<sycl::info::device::vendor>()
                     << nl;
                Info << "  Max compute units: "
                     << device.get_info<sycl::info::device::max_compute_units>()
                     << nl;
                Info << "  Max work-group size: "
                     << device.get_info<sycl::info::device::max_work_group_size>()
                     << nl << nl;
            }
            catch (const sycl::exception& e)
            {
                FatalErrorInFunction
                    << "SYCL device initialization failed: " << e.what()
                    << abort(FatalError);
            }

            initDeviceFlag_ = true;
        }
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
