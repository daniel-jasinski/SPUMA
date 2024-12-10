#ifndef Foam_cuda_executor_cu
#define Foam_cuda_executor_cu


#include "cudaExecutor.H"
#include "deviceM.H"
#include "deviceUtils.H"
#include "Atomic.H"
#include "cudaError.H"

namespace Foam
{

namespace cuda
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


} // namespace cuda


} // namespace foam

template<typename F>
void Foam::cudaExecutor::_backendFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    label numblocks = SET_NUM_BLOCKS(size);
    //numblocks = numblocks == 0 ? 1 : numblocks;

    Foam::cuda::lambdaKernel<F>
    //<<<(numblocks + NUM_SM -1)/ NUM_SM,NUM_THREADS_PER_BLOCK>>>
    <<<numblocks,NUM_THREADS_PER_BLOCK>>>
    (lambda,size);

    deviceSync(); 
    CHECK_LAST_CUDA_ERROR();
};

template<typename F>
void Foam::cudaExecutor::_backendSerialFor(F& lambda, const label& size)
{
    if (size <= 0)
        return;

    Foam::cuda::lambdaKernel<F><<<1,1>>>(lambda,size);

    deviceSync(); 
    CHECK_LAST_CUDA_ERROR();
};

template <typename F,typename resultT>
void Foam::cudaExecutor::_backendReductionSum(
    F& lambda,
    resultT* const __restrict__ result,
    const label& size
)
{
    if (size <= 0) return;

    resultT* dPtrResult;
    CHECK_CUDA_ERROR(cudaMalloc(&dPtrResult,sizeof(resultT)));
    CHECK_CUDA_ERROR(cudaMemcpyAsync(dPtrResult,result,sizeof(resultT),cudaMemcpyHostToDevice));
    // create mutex
    Foam::Mutex mutex;

    const label numBlocks = SET_TREE_REDUCE_NUM_BLOCKS(size);

    int maxbytes = MAX_SMEM; 
    // declare that this kernel can use up to MAX_SMEM of dynamically allocated shared memory
    CHECK_CUDA_ERROR
    (
        cudaFuncSetAttribute
        (
            Foam::cuda::reductionLambdaSumKernel<resultT,F>,
            cudaFuncAttributeMaxDynamicSharedMemorySize, 
            maxbytes
        )
    );

    Foam::cuda::reductionLambdaSumKernel<resultT,F>
    <<<(numBlocks + NUM_SM -1)/NUM_SM,NUM_THREADS_PER_BLOCK,maxbytes>>>
    (
        dPtrResult,
        lambda,
        mutex.getMutex(),
        size
    );

    //deviceSync(); 
    CHECK_LAST_CUDA_ERROR();

    CHECK_CUDA_ERROR(cudaMemcpyAsync(result,dPtrResult,sizeof(resultT),cudaMemcpyDeviceToHost));
    CHECK_CUDA_ERROR(cudaFree(dPtrResult));
};

template <typename F,typename Op,typename resultT>
void Foam::cudaExecutor::_backendReductionCompare(
    F& lambda,
    Op& op,
    resultT* const __restrict__ result,
    const label& size
)
{
    if (size <= 0) return;

    resultT* dPtrResult;
    CHECK_CUDA_ERROR(cudaMalloc(&dPtrResult,sizeof(resultT)));
    CHECK_CUDA_ERROR(cudaMemcpyAsync(dPtrResult,result,sizeof(resultT),cudaMemcpyHostToDevice));
    // create mutex
    Foam::Mutex mutex;

    const label numBlocks = SET_TREE_REDUCE_NUM_BLOCKS(size);

    int maxbytes = MAX_SMEM; 
    // declare that this kernel can use up to MAX_SMEM of dynamically allocated shared memory
    CHECK_CUDA_ERROR
    (
        cudaFuncSetAttribute
        (
            Foam::cuda::reductionLambdaCompareKernel<resultT,F,Op>,
            cudaFuncAttributeMaxDynamicSharedMemorySize, 
            maxbytes
        )
    );

    Foam::cuda::reductionLambdaCompareKernel<resultT,F,Op>
    <<<(numBlocks + NUM_SM -1)/NUM_SM,NUM_THREADS_PER_BLOCK,maxbytes>>>
    (
        dPtrResult,
        lambda,
        op,
        mutex.getMutex(),
        size
    );

    //deviceSync(); 
    CHECK_LAST_CUDA_ERROR();

    CHECK_CUDA_ERROR(cudaMemcpyAsync(result,dPtrResult,sizeof(resultT),cudaMemcpyDeviceToHost));
    CHECK_CUDA_ERROR(cudaFree(dPtrResult));
};


#endif
