/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/
#include "hipError.hpp"

template <typename T>
int checkHipError
(
    T err,
    const char* const func,
    const char* const file,
    const int line
)
{
    if (err != hipSuccess)
    {
        std::cerr << "HIP Runtime Error at: " << file << ":" << line
                  << std::endl;
        std::cerr << hipGetErrorString(err) << " " << func << std::endl;

        FatalErrorInFunction<<abort(Foam::FatalError);
    }
    return static_cast<int>(err);
}

int checkLastHipError
(
    const char* const file,
    const int line
)
{
    hipError_t err(hipGetLastError());
    if (err != hipSuccess)
    {
        std::cerr << "HIP Runtime Error at: " << file << ":" << line
                  << std::endl;
        std::cerr << hipGetErrorString(err) << std::endl;

        FatalErrorInFunction<<abort(Foam::FatalError);
    }
    return static_cast<int>(err);
}

bool isDeviceValid(const void * ptr)
{
    bool valid = false;

    hipPointerAttribute_t attr;
    CHECK_HIP_ERROR(hipPointerGetAttributes(&attr,(void*)ptr));
    if(attr.devicePointer)
        valid = true;

    return valid;
}

template int checkHipError<hipError_t>(hipError_t err, const char* const func,
                                          const char* const file, const int line);

