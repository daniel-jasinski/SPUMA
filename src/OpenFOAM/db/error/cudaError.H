/*---------------------------------------------------------------------------*\
-------------------------------------------------------------------------------
    Copyright (C) 2023-2024 CINECA
-------------------------------------------------------------------------------
License
    This file is part of zeptoFOAM.

    zeptoFOAM is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    zeptoFOAM is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with zeptoFOAM.
    If not, see <http://www.gnu.org/licenses/>.

\*---------------------------------------------------------------------------*/

#ifndef cudaError_H
#define cudaError_H

#include <iostream>
#include <cuda.h>
#include <cuda_runtime_api.h>
#include <cusparse.h>

#define NUM_THREADS_PER_BLOCK 128
#define CUDA_MEM_ALIGN_BYTES 16

#define SET_NUM_BLOCKS(size)           \
    (size + NUM_THREADS_PER_BLOCK - 1) \
    / NUM_THREADS_PER_BLOCK
// #define SET_NUM_BLOCKS(size) 128

#define SET_NUM_BLOCKS_MULTI(size, multiple)    \
    (size + multiple*NUM_THREADS_PER_BLOCK - 1) \
    / (multiple*NUM_THREADS_PER_BLOCK)
// #define SET_NUM_BLOCKS(size) 128

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

template<typename T>
int checkCusparse(T f, const char* const func, const char* const file, const int line)
{
    const cusparseStatus_t __s = f;

    if(__s != CUSPARSE_STATUS_SUCCESS)
    {
        std::cerr << "CUDA status in: " << f << " [" << __FILE__ << ","
            << __LINE__ << "], " << std::endl;
        exit(1);
    }
    return static_cast<int>(__s);
}
#define CHECK_CUSPARSE(val) checkCusparse((val), #val, __FILE__, __LINE__)

#endif

