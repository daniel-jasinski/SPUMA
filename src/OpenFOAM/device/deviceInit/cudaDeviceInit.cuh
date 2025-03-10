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
public:

    static void _backendInit()
    {
        if (!initDeviceFlag_)
        {
            Info << "Initializing CUDA devices..." << nl << nl;

            label nDevs;
            cudaGetDeviceCount(&nDevs);

            label devID = Pstream::myProcNo() % nDevs;
            cudaSetDevice(devID);

            initDeviceFlag_ = true;
        }
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
