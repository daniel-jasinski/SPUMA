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

#include "cudaError.cuh"

template <typename T>
int checkCudaError
(
    T err,
    const char* const func,
    const char* const file,
    const int line
)
{
    if (err != cudaSuccess)
    {
        std::cerr << "CUDA Runtime Error at: " << file << ":" << line
                  << std::endl;
        std::cerr << cudaGetErrorString(err) << " " << func << std::endl;

        FatalErrorInFunction<<abort(Foam::FatalError);
    }
    return static_cast<int>(err);
}

int checkLastCudaError
(
    const char* const file,
    const int line
)
{
    cudaError_t err(cudaGetLastError());
    if (err != cudaSuccess)
    {
        std::cerr << "CUDA Runtime Error at: " << file << ":" << line
                  << std::endl;
        std::cerr << cudaGetErrorString(err) << std::endl;

        FatalErrorInFunction<<abort(Foam::FatalError);
    }
    return static_cast<int>(err);
}

bool isDeviceValid(const void * ptr)
{
    bool valid = false;

    cudaPointerAttributes attr;
    CHECK_CUDA_ERROR(cudaPointerGetAttributes(&attr,(void*)ptr));
    if(attr.devicePointer)
        valid = true;

    return valid;
}

template int checkCudaError<cudaError_t>
(
    cudaError_t err,
    const char* const func,
    const char* const file, const int line
);

// ************************************************************************* //
