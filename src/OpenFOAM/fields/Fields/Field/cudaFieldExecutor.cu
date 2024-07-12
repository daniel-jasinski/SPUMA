#ifndef Foam_Cuda_field_Executor_C
#define Foam_Cuda_field_Executor_C

#include "cudaFieldExecutor.cuh"
#include "executorOps.H"
#include "deviceM.H"
#include "cudaError.H"
#include "uLabel.H"
#include "deviceUtils.H"
#include <cmath>
#include <cuda_runtime_api.h>

namespace Foam
{
//kernels
namespace cuda
{

//kernels
//NOTE array are accesed via pointers, small const variables are passed by value (copied)
// max limit size arguments for kernel is 4kb cuda<12.1, 32kb cuda>=12.1, they reside in 
// constant memory space of the GPU

template<typename Type1,typename Op>
__global__
void opKernel
(
    Type1* const __restrict__ fp,
    Op op,
    const label loop_len
)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len)
        op(fp[i],fp[i]);
};

template <typename Type1, typename Type2,
typename Op >
__global__
void opKernel
(
    Type1* const __restrict__ f1p,
    const Type2* const __restrict__ f2p,
    Op op,
    const label loop_len
)
{   
    unsigned int id = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int gridSize = blockDim.x*gridDim.x;

    // if (id < loop_len)
    while(id < loop_len) //WIP
    {
        op(f1p[id],f2p[id]);
        id+=gridSize;
    }
};

template <typename Type1, typename Type2,
typename Op >
__global__
void opKernel
(
    Type1* const __restrict__ f1p,
    const Type2 s,
    Op op,
    const label loop_len
)
{   
          unsigned int id = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int gridSize = blockDim.x*gridDim.x;

    // if (id < loop_len)
    while(id < loop_len) //WIP
    {
        op(f1p[id],s);
        id+=gridSize;
    }
};

template <typename resultType, typename Type1, typename Type2,
typename Op >
__global__
void opKernel
(
    resultType* const __restrict__ resultp,
    const Type1* const __restrict__ f1p,
    const Type2* const __restrict__ f2p,
    Op op,
    const label loop_len
)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len)
        op(resultp[i],f1p[i], f2p[i]);
};

template <typename resultType, typename Type1, typename Type2,
typename Op >
__global__
void opKernel
(
    resultType* const __restrict__ resultp,
    const Type1 s,
    const Type2* const __restrict__ fp,
    Op op,
    const label loop_len
)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len)
        op(resultp[i],s, fp[i]);
};

template <typename resultType, typename Type1, typename Type2,
typename Op >
__global__
void opKernel
(
    resultType* const __restrict__ resultp,
    const Type1* const __restrict__ fp,
    const Type2 s,
    Op op,
    const label loop_len
)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len)
        op(resultp[i], fp[i], s);
};

template <typename resultType, typename Type1, typename Type2, typename Type3,
typename Op >
__global__
void opKernel
(
    resultType* const __restrict__ resultp,
    const Type1* const __restrict__ f1p,
    const Type2* const __restrict__ f2p,
    const Type3* const __restrict__ f3p,
    Op op,
    const label loop_len
)
{   
          unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int gridSize = blockDim.x * gridDim.x;
    while (i < loop_len){
        op(resultp[i],f1p[i], f2p[i], f3p[i]);
        i += gridSize;
    }
};

template <typename resultType, typename Type1, typename Type2, typename Type3,
typename Op >
__global__
void opKernel
(
    resultType* const __restrict__ resultp,
    const Type1* const __restrict__ f1p,
    const Type2* const __restrict__ f2p,
    const Type3 s,
    Op op,
    const label loop_len
)
{   
          unsigned int i = blockIdx.x * blockDim.x + threadIdx.x;
    const unsigned int gridSize = blockDim.x * gridDim.x;
    while (i < loop_len){
        op(resultp[i],f1p[i], f2p[i], s);
        i += gridSize;
    }
};

// reduction kernels

template <typename resultType, typename Type1, typename Type2,
typename Op >
__global__
void reductionSumKernel
(
    resultType* const __restrict__ result,
    const Type1* const __restrict__ f1p,
    const Type2* const __restrict__ f2p,
    Op op,
    const label loop_len
)
{   
          unsigned int id  = blockIdx.x * (2*blockDim.x) + threadIdx.x; // global id
    const unsigned int tid = threadIdx.x; //block local thread id
    const label gsize = loop_len;
    const unsigned int blockSize = NUM_THREADS_PER_BLOCK; 
    const unsigned int gridSize = blockDim.x*2*gridDim.x; //number of thread in a grid

    __shared__ resultType sdata[NUM_THREADS_PER_BLOCK]; //static array in shared memory where the redution is computed
    sdata[tid] = resultType(Zero); //initialize local value to zero

    //load from gloabl memory + gsize/gridSize step of reduction 
    while (id < gsize){
        //sdata[tid] += op(f1p[id],f2p[id]) + op(f1p[id+blockSize],f2p[id+blockSize]);
        if (id+blockSize < gsize){
            sdata[tid] += op(f1p[id],f2p[id]) + op(f1p[id+blockSize],f2p[id+blockSize]);
        }else{
            sdata[tid] += op(f1p[id],f2p[id]);
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
        warpReduce<resultType,blockSize>(sdata, tid);
    }

    // global accumulator
    if (tid == 0)
    {
        atomicAdd(result, sdata[0]); //each "master" rank 0 thread add its value to the global variable
    }
};


//NOTE: global accumulator handled with a mutex
template <typename resultType, typename Type1,
typename Op >
__global__
void reductionSumKernel
(
    resultType* const __restrict__ result,
    const Type1* const __restrict__ f1p,
    Op op,
    spinLock& lock,
    const label loop_len
)
{
          unsigned int id  = blockIdx.x * (2*blockDim.x) + threadIdx.x; // global id
    const unsigned int tid = threadIdx.x; //block local thread id
    const label gsize = loop_len;
    const unsigned int blockSize = NUM_THREADS_PER_BLOCK; 
    const unsigned int gridSize = blockDim.x*2*gridDim.x; //number of thread in a grid

    __shared__ resultType sdata[NUM_THREADS_PER_BLOCK]; //static arry in shared memory where the redution is computed
    sdata[tid] = resultType(Zero); //initialize array

    __threadfence();
    __syncthreads();
    //grid-wise reduction step
    //load from gloabl memory + gsize/gridSize step of reduction 
    while (id < gsize){
        //handle loop_len not multiple of blockSize
        if (id+blockSize < gsize){
            sdata[tid] += op(f1p[id]) + op(f1p[id+blockSize]);
        }else{
            sdata[tid] += op(f1p[id]);
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
    // note for high number of block too much contention of the mutex!
    if(tid == 0)
    {
        lock.lock();
        __threadfence();
        *result += sdata[0];
        __threadfence();
        lock.unlock();
    }

};

};// end namespace cuda

};


/*---------------- Memeber functions ----------------------------------------------- */

template<typename Op>
void Foam::cudaFieldExecutor<Op>::opF_OP_F
(
    resultType* resultPtr,
    Op op,
    const label loop_len
)
{  
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  

    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents);


    Foam::cuda::opKernel<resultT,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            op,
            loop_len*op.nComponents
        );
    deviceSync();

    CHECK_LAST_CUDA_ERROR();
    //Info << "EXEC: ciao cuda op" << endl;

};

template<typename Op>
void Foam::cudaFieldExecutor<Op>::opF_OP_F
(
    resultType* resultPtr,
    const Type1* field1Ptr,
    Op op,
    const label loop_len
)
{  
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1 kernelType;
    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  
    const kernelType* const f1p = reinterpret_cast<const kernelType*>(field1Ptr);  

    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents);


    Foam::cuda::opKernel<resultT,kernelType,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            f1p,
            op,
            loop_len*op.nComponents
        );
    deviceSync();

    CHECK_LAST_CUDA_ERROR();
    //Info << "EXEC: ciao cuda op" << endl;

};


template<typename Op>
void Foam::cudaFieldExecutor<Op>::opF_OP_S
(
    resultType* resultPtr,
    const Type1& cmptRef,
    Op op,
    const label loop_len
)
{
    // this is a redundant type casting since at the moment ops are parallelized on multiple components
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1   refT;
    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  
    const refT& cmptref = reinterpret_cast<const refT&>(cmptRef);  

    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents); //eventualy insert Ncomponents?

    Foam::cuda::opKernel<resultT,refT,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            cmptref,
            op,
            loop_len*op.nComponents
        );
    
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
}

template<typename Op>
void Foam::cudaFieldExecutor<Op>::opF_OP_F
(
    resultType* resultPtr,
    const Type1* field1Ptr,
    const Type2* field2Ptr,
    Op op,
    const label loop_len
)
{  
    // appropriate pointer casting
    typedef typename Op::resultT resultKernelT;
    typedef typename Op::Type1 kernelType1;
    typedef typename Op::Type2 kernelType2;
    resultKernelT* const resultp = reinterpret_cast<resultKernelT*>(resultPtr);  
    const kernelType1* const f1p = reinterpret_cast<const kernelType1*>(field1Ptr);  
    const kernelType2* const f2p = reinterpret_cast<const kernelType2*>(field2Ptr);  

    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents);

    Foam::cuda::opKernel<resultKernelT,kernelType1,kernelType2,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            f1p,
            f2p,
            op,
            loop_len*op.nComponents
        );
    deviceSync();

    CHECK_LAST_CUDA_ERROR();

};

template<typename Op>
void Foam::cudaFieldExecutor<Op>::opS_OP_F
(
    resultType* resultPtr,
    const Type1 &cmptRef,
    const Type2* fieldPtr,
    Op op,
    const label loop_len
)
{
    // this is a redundant type casting since at the moment ops are parallelized on multiple components
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1   refT;
    typedef typename Op::Type2   T2;
    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  
    const refT& cmptref = reinterpret_cast<const refT&>(cmptRef);  
    const T2* const fp = reinterpret_cast<const T2*>(fieldPtr);  
    
    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents); //eventualy insert Ncomponents?

    Foam::cuda::opKernel<resultT,refT,T2,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            cmptref,
            fp,
            op,
            loop_len*op.nComponents
        );
    
    deviceSync();
    CHECK_LAST_CUDA_ERROR();

};

template<typename Op>
void Foam::cudaFieldExecutor<Op>::opF_OP_S
(
    resultType* resultPtr,
    const Type1* field2Ptr,
    const Type2 &cmptRef,
    Op op,
    const label loop_len
)
{
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1   T1;
    typedef typename Op::Type2   refT;

    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  
    const T1* const fp =     reinterpret_cast<const T1*>(field2Ptr);  
    const refT& cmptref =    reinterpret_cast<const refT&>(cmptRef);  
    
    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents);

    Foam::cuda::opKernel<resultT,T1,refT,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            fp,
            cmptref,
            op,
            loop_len*op.nComponents
        );
    deviceSync();    
    CHECK_LAST_CUDA_ERROR();

};

template<typename Op>
void Foam::cudaFieldExecutor<Op>::opF_OP_F_F
(
    resultType* resultPtr,
    const Type1* field1Ptr,
    const Type2* field2Ptr,
    const Type3* field3Ptr,
    Op op,
    const label loop_len
)
{
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1   T1;
    typedef typename Op::Type2   T2;
    typedef typename Op::Type3   T3;

    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  
    const T1* const f1p =    reinterpret_cast<const T1*>(field1Ptr);  
    const T2* const f2p =    reinterpret_cast<const T1*>(field2Ptr);  
    const T3* const f3p =    reinterpret_cast<const T3*>(field3Ptr);  
    
    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents);

    Foam::cuda::opKernel<resultT,T1,T2,T3,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            f1p,
            f2p,
            field3Ptr,
            op,
            loop_len*op.nComponents
        );
    deviceSync();    
    CHECK_LAST_CUDA_ERROR();

};

template<typename Op>
void Foam::cudaFieldExecutor<Op>::opF_OP_F_S
(
    resultType* resultPtr,
    const Type1* field1Ptr,
    const Type2* field2Ptr,
    const Type3 &cmptRef,
    Op op,
    const label loop_len
)
{
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1   T1;
    typedef typename Op::Type2   T2;
    //typedef typename Op::Type3   T3;

    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  
    const T1* const f1p =    reinterpret_cast<const T1*>(field1Ptr);  
    const T2* const f2p =    reinterpret_cast<const T2*>(field2Ptr);  
    //const T3& cmptref =    reinterpret_cast<const T3&>(cmptRef); 
    
    // calc number of block to dispatch
    const label numBlocks = SET_NUM_BLOCKS(loop_len*op.nComponents);

    Foam::cuda::opKernel<resultT,T1,T2,scalar,Op>
        <<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            resultp,
            f1p,
            f2p,
            cmptRef,
            op,
            loop_len*op.nComponents
        );
    deviceSync();    
    CHECK_LAST_CUDA_ERROR();

};

template <typename Op>
void Foam::cudaFieldExecutor<Op>::reductionSum
(
    resultType &result,
    const Type1 *field1Ptr,
    const Type2 *field2Ptr,
    Op op,
    const label loop_len
)
{

    typedef typename Op::resultT resultT;
    typedef typename Op::Type1 T1;
    typedef typename Op::Type2 T2;

    resultT &resultRef = reinterpret_cast<resultT &>(result);
    const T1 *const f1p = reinterpret_cast<const T1 *>(field1Ptr);
    const T2 *const f2p = reinterpret_cast<const T2 *>(field2Ptr);

    CHECK_CUDA_ERROR(cudaHostRegister(&resultRef,sizeof(resultT),cudaHostRegisterDefault));
    
    const label numBlocks = SET_TREE_REDUCE_NUM_BLOCKS(loop_len);

    Foam::cuda::reductionSumKernel<resultType, T1, T2, Op>
        //<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        <<<(numBlocks/NUM_SM + NUM_SM), NUM_THREADS_PER_BLOCK>>>
        (
            &resultRef,
            f1p,
            f2p,
            op,
            loop_len * op.nComponents
        );
    deviceSync();
    CHECK_LAST_CUDA_ERROR();
    CHECK_CUDA_ERROR(cudaHostUnregister(&resultRef));
};


template<typename Op>
void Foam::cudaFieldExecutor<Op>::reductionSum
(
    resultType& result,
    const Type1* field1Ptr,
    Op op,
    const label loop_len 
)
{
    //Info<< "CUDA redux" <<endl;
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1   T1;

    resultT& resultRef = reinterpret_cast<resultT&>(result);  
    const T1* const f1p = reinterpret_cast<const T1*>(field1Ptr);  

    const label numBlocks = SET_TREE_REDUCE_NUM_BLOCKS(loop_len);
    
    CHECK_CUDA_ERROR(cudaHostRegister(&resultRef,sizeof(resultT),cudaHostRegisterDefault));

    // create lock
    Foam::cuda::spinLock lock;

    Foam::cuda::reductionSumKernel<resultType,T1,Op>
        <<<(numBlocks/NUM_SM + NUM_SM), NUM_THREADS_PER_BLOCK>>>
        //<<<numBlocks, NUM_THREADS_PER_BLOCK>>>
        (
            &resultRef,
            f1p,
            op,
            lock,
            loop_len*op.nComponents
        );
    deviceSync(); 
    CHECK_LAST_CUDA_ERROR();


    CHECK_CUDA_ERROR(cudaHostUnregister(&resultRef));
}

#endif
