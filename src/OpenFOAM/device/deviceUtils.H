#ifndef Foam_Device_Utils_H
#define Foam_Device_Utils_H
#include "cudaError.H"
#include <cuda_runtime_api.h>

namespace Foam
{
namespace cuda
{

//- naive spinLock implementation
/*struct spinLock
{
    spinLock(){
        CHECK_CUDA_ERROR(cudaMalloc(&mutex_,sizeof(int)));
        CHECK_CUDA_ERROR(cudaMemset(mutex_,0,sizeof(int)));
    }

    ~spinLock(){
        CHECK_CUDA_ERROR(cudaFree(mutex_)); 
    }

    __device__ inline void lock() {
        while (atomicCAS((int*)mutex_, 0, 1) == 1) {};
    }
    
    // __device__ inline void exp_backoff_lock() {
    //     unsigned int ns = 8;
    //     while (atomicCAS((int*)mutex_, 0, 1) == 1) {
    //         __nanosleep(ns);
    //         if (ns < 256) {
    //            ns *= 2;
    //         }
    //     }
    // }

    __device__ inline void unlock() {
        atomicExch((int*)mutex_, 0);
    }

private:
    int* mutex_;
};*/


struct Mutex
{
    Mutex(){
        CHECK_CUDA_ERROR(cudaMalloc(&mutex_,sizeof(int)));
        CHECK_CUDA_ERROR(cudaMemset(mutex_,0,sizeof(int)));
    }

    ~Mutex(){
        CHECK_CUDA_ERROR(cudaFree(mutex_));
    }

    __host__ __device__ inline int* getMutex()
    {
        return mutex_;
    }

private:
    int* mutex_;
};

struct spinLock
{
    __device__ static inline void lock(int* mutex) {
        while (atomicCAS(mutex, 0, 1) == 1) {};
    }
    __device__ static inline void unlock(int* mutex){
       atomicExch(mutex, 0); 
    } 
};

template <typename T,int blockSize>
__device__
void warpReduceNoVolatile( T* sdata, int tid)
{   
    __threadfence_block();
    if (blockSize >= 64) { sdata[tid] += sdata[tid + 32];  __threadfence_block(); }
    if (blockSize >= 32) { sdata[tid] += sdata[tid + 16];  __threadfence_block(); }
    if (blockSize >= 16) { sdata[tid] += sdata[tid + 8] ;  __threadfence_block();  }
    if (blockSize >= 8)  { sdata[tid] += sdata[tid + 4] ;  __threadfence_block();  }
    if (blockSize >= 4)  { sdata[tid] += sdata[tid + 2] ;   __threadfence_block(); }
    if (blockSize >= 2)  { sdata[tid] += sdata[tid + 1] ;  __threadfence_block();  }
};


template <typename T,typename Op,int blockSize>
__device__
void warpReduceNoVolatile( T* sdata, Op& op,int tid)
{   
    __threadfence_block();
    if (blockSize >= 64) { sdata[tid] = op(sdata[tid],sdata[tid + 32]);  __threadfence_block(); }
    if (blockSize >= 32) { sdata[tid] = op(sdata[tid],sdata[tid + 16]);  __threadfence_block(); }
    if (blockSize >= 16) { sdata[tid] = op(sdata[tid],sdata[tid + 8] );  __threadfence_block();  }
    if (blockSize >= 8)  { sdata[tid] = op(sdata[tid],sdata[tid + 4] );  __threadfence_block();  }
    if (blockSize >= 4)  { sdata[tid] = op(sdata[tid],sdata[tid + 2] );   __threadfence_block(); }
    if (blockSize >= 2)  { sdata[tid] = op(sdata[tid],sdata[tid + 1] );  __threadfence_block();  }
};

template <typename T,int blockSize>
__device__
void newWarpReduceNoVolatile( T* sdata, int tid)
{   
    //cuda memory model do not guarantee that reads are perfomed before write: separate them
    T temp(Zero);
    __syncthreads();
    if (blockSize >= 64) {
        temp += sdata[tid + 32]; __syncthreads();
        sdata[tid] = temp; __syncthreads();
    }
    if (blockSize >= 32) {
        temp += sdata[tid + 16]; __syncthreads();
        sdata[tid] = temp; __syncthreads();
    }
    if (blockSize >= 16) {
        temp += sdata[tid + 8]; __syncthreads();
        sdata[tid] = temp; __syncthreads();
    }
    if (blockSize >= 8) {
        temp += sdata[tid + 4]; __syncthreads();
        sdata[tid] = temp; __syncthreads();
    }
    if (blockSize >= 4) {
        temp += sdata[tid + 2]; __syncthreads();
        sdata[tid] = temp; __syncthreads();
    }
    if (blockSize >= 2) {
        temp += sdata[tid + 1]; __syncthreads();
        sdata[tid] = temp; __syncthreads();
    }

};

template <typename T, int blockSize>
__device__
void warpReduce(volatile T* sdata, int tid) // volataile to ensure visibility of memory operations
{
    if (blockSize >= 64) sdata[tid] += sdata[tid + 32];
    if (blockSize >= 32) sdata[tid] += sdata[tid + 16];
    if (blockSize >= 16) sdata[tid] += sdata[tid + 8];
    if (blockSize >= 8)  sdata[tid] += sdata[tid + 4];
    if (blockSize >= 4)  sdata[tid] += sdata[tid + 2];
    if (blockSize >= 2)  sdata[tid] += sdata[tid + 1];
};


//- simple wrapper to allow use of extern linked shared memory by simply 
// casting the same pointer to the desired type. This prevent multiple definition
// of the same shared pointer of different type.
template <typename T>
struct SharedMemory{
  __device__ inline T *getPointer(){
    extern __shared__ __align__(8) char smem[];
    return reinterpret_cast<T*>(smem);
  }
};

}

}

#endif
