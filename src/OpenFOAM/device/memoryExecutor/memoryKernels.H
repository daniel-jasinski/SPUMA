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

Description
     Collection of memset kernels.

SourceFiles
    memoryKernels.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_memory_Kernels_H
#define Foam_memory_Kernels_H

// * * * * * * * * * * * * * * * * CUDA Kernels  * * * * * * * * * * * * * * //

namespace Foam
{

namespace device
{

__global__
void memSetKernel(uint64_t nElementsInBytes, int dataSize, char *target, const char *source)
{
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    const int stride = gridDim.x * blockDim.x;
    for(int ii = idx; ii < nElementsInBytes; ii += stride)
    {
        int jj = ii % dataSize;
        target[ii] = source[jj];
    }
};

__global__
void memSetOneKernel(uint64_t nElements, scalar *target)
{
    const scalar one = 1.0;
    const int idx = blockIdx.x * blockDim.x + threadIdx.x;
    const int stride = gridDim.x * blockDim.x;
    for(int ii = idx; ii < nElements; ii += stride)
    {
        target[ii] = one;
    }
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace device

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
