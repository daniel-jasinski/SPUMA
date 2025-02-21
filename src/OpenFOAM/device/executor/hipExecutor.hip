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
#ifndef Foam_hip_executor_cu
#define Foam_hip_executor_cu


#include "hipExecutor.hpp"
#include "deviceM.H"
#include "hipDeviceUtils.hpp"
#include "hipError.hpp"

namespace Foam
{

namespace hip
{
    template<typename F>
    __global__
    void lambdaKernel(F lambda, const label size)
    {
        unsigned int id  = blockIdx.x *blockDim.x + threadIdx.x; // global id
        const unsigned int gridSize = blockDim.x*gridDim.x; //number of thread in a grid

        //strided loop
        while (id < size)
        {
            lambda(id);
            id+=gridSize;
        }
    };

    template <typename resultType, typename F>
    __global__
    void reductionLambdaSumKernel
    (
        resultType* const __restrict__ result,
        F lambda,
        int* const __restrict__ mutex,
        const label size
    )
    {
            unsigned int id  = blockIdx.x * (2*blockDim.x) + threadIdx.x; // global id
        const unsigned int tid = threadIdx.x; //block local thread id
        const label gsize = size;
        const unsigned int blockSize = NUM_THREADS_PER_BLOCK;
        const unsigned int gridSize = blockDim.x*2*gridDim.x; //number of thread in a grid

        SharedMemory<resultType> smem;
        resultType *sdata = smem.getPointer();
        memset(&sdata[tid],0,sizeof(resultType));


        __syncthreads();
        //grid-wise reduction step
        //load from gloabl memory + gsize/gridSize step of reduction
        while (id < gsize){
            //handle loop_len not multiple of blockSize
            if (id+blockSize < gsize){
                sdata[tid] += lambda(id) + lambda(id+blockSize);
            }else{
                sdata[tid] += lambda(id);
            }
            id += gridSize;
        }
        __syncthreads();

        //block wise reduction steps
        if (blockSize >= 512) { if (tid < 256) { sdata[tid] += sdata[tid + 256]; } __syncthreads(); }
        if (blockSize >= 256) { if (tid < 128) { sdata[tid] += sdata[tid + 128]; } __syncthreads(); }
        if (blockSize >= 128) { if (tid < 64)  { sdata[tid] += sdata[tid + 64];  } __syncthreads(); }

        //warp wise reduction step
        //in warp: syncthread guaranteed
        if (tid < 32)
        {
            warpReduceNoVolatile<resultType,blockSize>(sdata, tid);
        }

        __syncthreads();
        if constexpr(std::is_same<resultType,double>::value){
            if(tid == 0)
            {
                atomicAdd(result,sdata[0]);
            }
        }else{
            // note for high number of block too much contention of the mutex!
            if(tid == 0)
            {
                spinLock::lock(mutex);
                __threadfence();
                *result += sdata[0];
                __threadfence();
                spinLock::unlock(mutex);
            }
        }

    };

    //room for improvement
    template <typename resultType, typename F, typename Op>
    __global__
    void reductionLambdaCompareKernel
    (
        resultType* const __restrict__ result,
        F lambda,
        Op op,
        int* const __restrict__ mutex,
        const label size
    )
    {
            unsigned int id  = blockIdx.x * (2*blockDim.x) + threadIdx.x; // global id
        const unsigned int tid = threadIdx.x; //block local thread id
        const label gsize = size;
        const unsigned int blockSize = NUM_THREADS_PER_BLOCK;
        const unsigned int gridSize = blockDim.x*2*gridDim.x; //number of thread in a grid

        SharedMemory<resultType> smem;
        resultType *sdata = smem.getPointer();
        resultType tmp;
        sdata[tid] = *result;

        __syncthreads();
        //grid-wise reduction step
        //load from gloabl memory + gsize/gridSize step of reduction
        while (id < gsize){
            //handle loop_len not multiple of blockSize
            if (id+blockSize < gsize){
                tmp = op(lambda(id),lambda(id+blockSize));
            }else{
                tmp = lambda(id);
            }
            sdata[tid]=op(tmp,sdata[tid]);
            id += gridSize;
        }
        __syncthreads();

        //block wise reduction steps
        if (blockSize >= 512) { if (tid < 256) {
            tmp = op(sdata[tid],sdata[tid + 256]); __threadfence_block();
            sdata[tid] = tmp;
        } __syncthreads(); }
        if (blockSize >= 256) { if (tid < 128) {
            tmp = op(sdata[tid],sdata[tid + 128]); __threadfence_block();
            sdata[tid] = tmp;
        } __syncthreads(); }
        if (blockSize >= 128) { if (tid < 64)  {
            tmp = op(sdata[tid],sdata[tid + 64]); __threadfence_block();
            sdata[tid] = tmp;
        } __syncthreads(); }

        //warp wise reduction step
        if (tid < 32)
        {
            warpReduceCompareNoVolatile<resultType,Op,blockSize>(sdata,op,tid);
        }

        __syncthreads();
        // note for high number of block too much contention of the mutex!
        if(tid == 0)
        {
            spinLock::lock(mutex);
            __threadfence();
            tmp = op(sdata[0],*result);
            __threadfence();
            *result = tmp;
            __threadfence();
            spinLock::unlock(mutex);
        }

    };


} // namespace hip


} // namespace foam

template<typename F>
void Foam::hipExecutor::_backendFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    label numblocks = SET_NUM_BLOCKS(size);
    //numblocks = numblocks == 0 ? 1 : numblocks;

    hipLaunchKernelGGL
    (
        Foam::hip::lambdaKernel<F>,
        numblocks,
        NUM_THREADS_PER_BLOCK,
        0,
        0,
        lambda,
        size
    );

    deviceSync();
    CHECK_LAST_HIP_ERROR();
};

template<typename F>
void Foam::hipExecutor::_backendSerialFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    hipLaunchKernelGGL
    (
        Foam::hip::lambdaKernel<F>,
        1,
        1,
        0,
        0,
        lambda,
        size
    );

    deviceSync();
    CHECK_LAST_HIP_ERROR();
};

template <typename F,typename resultT>
void Foam::hipExecutor::_backendReductionSum(
    F& lambda,
    resultT* const __restrict__ result,
    const label& size
)
{
    if (size <= 0) return;

    resultT* dPtrResult;
    CHECK_HIP_ERROR(hipMalloc(&dPtrResult,sizeof(resultT)));
    CHECK_HIP_ERROR(hipMemcpyAsync(dPtrResult,result,sizeof(resultT),hipMemcpyHostToDevice));
    // create mutex
    Foam::Mutex mutex;

    const label numBlocks = SET_TREE_REDUCE_NUM_BLOCKS(size);

    // int maxbytes = MAX_SMEM;
    // declare that this kernel can use up to MAX_SMEM of dynamically allocated shared memory
    // CHECK_HIP_ERROR
    // (
    //     hipFuncSetAttribute
    //     (
    //         Foam::hip::reductionLambdaSumKernel<resultT,F>,
    //         hipFuncAttributeMaxDynamicSharedMemorySize,
    //         maxbytes
    //     )
    // );

    // NOTE: for multiple template arguments in the kernel use parentheses or the
    //       preprocessor cannot parse the hipLaunchKernelGGL macro correctly
    hipLaunchKernelGGL
    (
        (Foam::hip::reductionLambdaSumKernel<resultT,F>),
        (numBlocks + NUM_SM -1)/NUM_SM,
        NUM_THREADS_PER_BLOCK,
        0, // maxbytes,
        0,
        dPtrResult,
        lambda,
        mutex.getMutex(),
        size
    );

    //deviceSync();
    CHECK_LAST_HIP_ERROR();

    CHECK_HIP_ERROR(hipMemcpyAsync(result,dPtrResult,sizeof(resultT),hipMemcpyDeviceToHost));
    CHECK_HIP_ERROR(hipFree(dPtrResult));
};

template <typename F,typename Op,typename resultT>
void Foam::hipExecutor::_backendReductionCompare(
    F& lambda,
    Op& op,
    resultT* const __restrict__ result,
    const label& size
)
{
    if (size <= 0) return;

    resultT* dPtrResult;
    CHECK_HIP_ERROR(hipMalloc(&dPtrResult,sizeof(resultT)));
    CHECK_HIP_ERROR(hipMemcpyAsync(dPtrResult,result,sizeof(resultT),hipMemcpyHostToDevice));
    // create mutex
    Foam::Mutex mutex;

    const label numBlocks = SET_TREE_REDUCE_NUM_BLOCKS(size);

    // int maxbytes = MAX_SMEM;
    // declare that this kernel can use up to MAX_SMEM of dynamically allocated shared memory
    // CHECK_HIP_ERROR
    // (
    //     hipFuncSetAttribute
    //     (
    //         Foam::hip::reductionLambdaCompareKernel<resultT,F,Op>,
    //         hipFuncAttributeMaxDynamicSharedMemorySize,
    //         maxbytes
    //     )
    // );

    // NOTE: for multiple template arguments in the kernel use parentheses or the
    //       preprocessor cannot parse the hipLaunchKernelGGL macro correctly
    hipLaunchKernelGGL
    (
        (Foam::hip::reductionLambdaCompareKernel<resultT,F,Op>),
        (numBlocks + NUM_SM -1)/NUM_SM,
        NUM_THREADS_PER_BLOCK,
        0, // maxbytes,
        0,
        dPtrResult,
        lambda,
        op,
        mutex.getMutex(),
        size
    );

    //deviceSync();
    CHECK_LAST_HIP_ERROR();

    CHECK_HIP_ERROR(hipMemcpyAsync(result,dPtrResult,sizeof(resultT),hipMemcpyDeviceToHost));
    CHECK_HIP_ERROR(hipFree(dPtrResult));
};


#endif
