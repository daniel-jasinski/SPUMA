#ifndef Foam_device_M_H
#define Foam_device_M_H

#ifdef have_cuda
    #define FOAM_DEVICE __host__ __device__
    #define FOAM_DEVICE_I __host__ __device__ __forceinline__
#else
    #define FOAM_DEVICE
    #define FOAM_DEVICE_I inline
#endif 


// utility
#ifdef have_cuda
    #include <cuda_runtime_api.h>
    #define deviceSync() cudaDeviceSynchronize()
#else 
    #define deviceSync() 
#endif

// number of thread per block
#define NUM_THREADS_PER_BLOCK 128
//number of streaming multiprocessor per device (124 on A100)
#define NUM_SM 124 

//- calc the number of block for a generic kernel
#define SET_NUM_BLOCKS(size)           \
    (size + NUM_THREADS_PER_BLOCK - 1) \
    / NUM_THREADS_PER_BLOCK

//- calc the number of block for binary tree reduction algorithm
#define SET_TREE_REDUCE_NUM_BLOCKS(size)           \
    (size + 2*NUM_THREADS_PER_BLOCK - 1) \
    / (2*NUM_THREADS_PER_BLOCK)


#endif
