#ifndef Foam_Device_Utils_H
#define Foam_Device_Utils_H
#ifdef have_cuda
#include "cudaError.cuh"
#include <cuda_runtime_api.h>

__device__ __forceinline__
double atomicMin(double *address, double val)
{
    unsigned long long ret = __double_as_longlong(*address);

    while(val < __longlong_as_double(ret))
    {
        unsigned long long old = ret;

        if
        (
            (
                ret = atomicCAS
                (
                    (
                        unsigned long long *)address,
                        old,
                        __double_as_longlong(val)
                )
            ) == old
        )
            break;
    }

    return __longlong_as_double(ret);
}

__device__ __forceinline__
double atomicMax(double *address, double val)
{
    unsigned long long ret = __double_as_longlong(*address);

    while(val > __longlong_as_double(ret))
    {
        unsigned long long old = ret;

        if
        (
            (
                ret = atomicCAS
                (
                    (unsigned long long *)address,
                    old,
                    __double_as_longlong(val)
                )
            ) == old
        )
           break;
    }

    return __longlong_as_double(ret);
}

namespace Foam
{


struct Mutex
{
    Mutex()
    {
        mutex_ = static_cast<int*>(MemoryPool::getInstance()->allocate(sizeof(int)));
        Foam::MemoryPool::getInstance()->memSet(mutex_, 0, sizeof(int));
    }

    ~Mutex()
    {
	MemoryPool::getInstance()->free(mutex_); 
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

namespace cuda
{

template <typename T,int blockSize>
__device__
void warpReduceNoVolatile( T* sdata, int tid)
{
    T tmp;
    if (blockSize >= 64) { tmp = sdata[tid + 32];  __syncwarp(); sdata[tid]+=tmp; __syncwarp(); }
    if (blockSize >= 32) { tmp = sdata[tid + 16];  __syncwarp(); sdata[tid]+=tmp; __syncwarp(); }
    if (blockSize >= 16) { tmp = sdata[tid +  8];  __syncwarp(); sdata[tid]+=tmp; __syncwarp(); }
    if (blockSize >=  8) { tmp = sdata[tid +  4];  __syncwarp(); sdata[tid]+=tmp; __syncwarp(); }
    if (blockSize >=  4) { tmp = sdata[tid +  2];  __syncwarp(); sdata[tid]+=tmp; __syncwarp(); }
    if (blockSize >=  2) { tmp = sdata[tid +  1];  __syncwarp(); sdata[tid]+=tmp; __syncwarp(); }
};


template <typename T,typename Op,int blockSize>
__device__
void warpReduceCompareNoVolatile( T* sdata, Op& op,int tid)
{
    T tmp;
    if (blockSize >= 64) { tmp = op(sdata[tid],sdata[tid + 32]);__syncwarp(); 
        sdata[tid]= tmp;  __syncwarp(); }
    if (blockSize >= 32) { tmp = op(sdata[tid],sdata[tid + 16]);__syncwarp(); 
        sdata[tid]= tmp;  __syncwarp(); }
    if (blockSize >= 16) { tmp = op(sdata[tid],sdata[tid + 8] );__syncwarp(); 
        sdata[tid]= tmp;   __syncwarp(); }
    if (blockSize >= 8)  { tmp = op(sdata[tid],sdata[tid + 4] ); __syncwarp();
        sdata[tid]= tmp;  __syncwarp(); }
    if (blockSize >= 4)  { tmp = op(sdata[tid],sdata[tid + 2] ); __syncwarp();
        sdata[tid]= tmp;   __syncwarp(); }
    if (blockSize >= 2)  { tmp = op(sdata[tid],sdata[tid + 1] ); __syncwarp();
        sdata[tid]= tmp;  __syncwarp();  }
};

template <typename T,int blockSize>
__device__
void newWarpReduceNoVolatile( T* sdata, int tid)
{   
    //cuda memory model do not guarantee that reads are perfomed before write: separate them
    
    T temp(Zero); //error static init
    //memset(temp.v_,0,sizeof(T));
    if (blockSize >= 64) {
        temp += sdata[tid + 32]; __syncwarp();
        sdata[tid] = temp; __syncwarp();
    }
    if (blockSize >= 32) {
        temp += sdata[tid + 16]; __syncwarp();
        sdata[tid] = temp; __syncwarp();
    }
    if (blockSize >= 16) {
        temp += sdata[tid + 8]; __syncwarp();
        sdata[tid] = temp; __syncwarp();
    }
    if (blockSize >= 8) {
        temp += sdata[tid + 4]; __syncwarp();
        sdata[tid] = temp; __syncwarp();
    }
    if (blockSize >= 4) {
        temp += sdata[tid + 2]; __syncwarp();
        sdata[tid] = temp; __syncwarp();
    }
    if (blockSize >= 2) {
        temp += sdata[tid + 1]; __syncwarp();
        sdata[tid] = temp; __syncwarp();
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

} // end namespace cuda

} //end namespace Foam

#endif

#endif
