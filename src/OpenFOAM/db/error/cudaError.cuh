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
    Defines macros for kernel launch setup and error handling in Cuda.

\*---------------------------------------------------------------------------*/

#ifndef cudaError_H
#define cudaError_H

#include "error.H"
#include <iostream>
#include <cuda.h>
#include <cuda_runtime_api.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define NUM_THREADS_PER_BLOCK 128
#define CUDA_MEM_ALIGN_BYTES 16

#define SET_NUM_BLOCKS(size)           \
    (size + NUM_THREADS_PER_BLOCK - 1) \
    / NUM_THREADS_PER_BLOCK

#define SET_NUM_BLOCKS_MULTI(size, multiple)    \
    (size + multiple*NUM_THREADS_PER_BLOCK - 1) \
    / (multiple*NUM_THREADS_PER_BLOCK)

int checkLastCudaError
(
    const char* const file,
    const int line
);
#define CHECK_LAST_CUDA_ERROR() checkLastCudaError(__FILE__, __LINE__)

template <typename T>
int checkCudaError
(
    T err,
    const char* const func,
    const char* const file,
    const int line
);
#define CHECK_CUDA_ERROR(val) checkCudaError((val), #val, __FILE__, __LINE__)

// check if a pointer is a valid gpu pointer;
bool isDeviceValid(const void * ptr);

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

