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
    Global SYCL queue instance for SPUMA.
    Uses GPU device if available, falls back to CPU.

\*---------------------------------------------------------------------------*/

#ifdef have_sycl

#include <sycl/sycl.hpp>
#include "syclDeviceInit.H"

namespace Foam
{

// Static member definitions
int syclDeviceInit::nThreadsPerBlock_ = 256;

void syclDeviceInit::_setNumberOfThreadsPerBlock(const int n)
{
    nThreadsPerBlock_ = n;
}

int syclDeviceInit::_getNumberOfThreadsPerBlock()
{
    return nThreadsPerBlock_;
}

// Global SYCL queue - heap-allocated for explicit lifetime control.
// Destroyed by destroySyclQueue() before process exit to ensure
// AdaptiveCpp cleanup runs while CUDA/HIP runtime is still alive.
static sycl::queue* syclQueuePtr_ = nullptr;
static bool syclQueueDestroyed_ = false;

sycl::queue& getSyclQueue()
{
    if (!syclQueuePtr_)
    {
        if (syclQueueDestroyed_)
        {
            // Re-creating a queue after shutdown would hand out a new
            // context while earlier allocations belong to the destroyed
            // one - surface the logic error instead of silent UB.
            FatalErrorInFunction
                << "SYCL queue accessed after device shutdown"
                << abort(FatalError);
        }
        try
        {
            syclQueuePtr_ = new sycl::queue(sycl::gpu_selector_v);
        }
        catch (const sycl::exception&)
        {
            syclQueuePtr_ = new sycl::queue(sycl::default_selector_v);
        }
    }
    return *syclQueuePtr_;
}

bool syclQueueActive()
{
    return syclQueuePtr_ != nullptr;
}

bool syclQueueShutdown()
{
    return syclQueueDestroyed_;
}

void destroySyclQueue()
{
    if (syclQueuePtr_)
    {
        syclQueuePtr_->wait();
        delete syclQueuePtr_;
        syclQueuePtr_ = nullptr;
    }
    syclQueueDestroyed_ = true;
}

} // End namespace Foam

#endif // have_sycl

// ************************************************************************* //
