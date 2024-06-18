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
    #define hostRegister(ptr, size) (cudaHostRegister( (void*)(ptr), (size), cudaHostRegisterReadOnly))
    #define hostUnregister(ptr) (cudaHostUnregister((void*)(ptr)))
#else 
    #define deviceSync() 
    #define hostRegister(ptr, size)
    #define hostUnregister(ptr) 
#endif

#define NUM_THREADS_PER_BLOCK 128
#define SET_NUM_BLOCKS(size)           \
    (size + NUM_THREADS_PER_BLOCK - 1) \
    / NUM_THREADS_PER_BLOCK

#endif
