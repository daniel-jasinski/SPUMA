#ifndef Foam_device_M_H
#define Foam_device_M_H

#if defined(have_cuda) || defined(have_hip)
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
#elif have_hip
    #include <hip/hip_runtime.h>
    #define deviceSync() hipDeviceSynchronize()
#else
    #define deviceSync()
#endif


#ifdef have_cuda

// Number of threads per block
#define NUM_THREADS_PER_BLOCK 128

// Number of streaming multiprocessor per device (124 on A100)
#define NUM_SM 124

// Max shared memory addressable by a block on c.u 8.0, 132 kB
#define MAX_SMEM 135168

#elif have_hip

// Number of threads per block
#define NUM_THREADS_PER_BLOCK 1024

// Number of streaming multiprocessor per device
#define NUM_SM 110

// Max shared memory addressable by a block
#define MAX_SMEM 65536

#endif

//- calc the number of block for a generic kernel
#define SET_NUM_BLOCKS(size)           \
    (size + NUM_THREADS_PER_BLOCK - 1) \
    / NUM_THREADS_PER_BLOCK

//- calc the number of block for binary tree reduction algorithm
#define SET_TREE_REDUCE_NUM_BLOCKS(size)           \
    (size + 2*NUM_THREADS_PER_BLOCK - 1) \
    / (2*NUM_THREADS_PER_BLOCK)


// Macro for loops over cells with ldu addressing
#define forAllOwner(ownStart,idCell,idFace,expression) \
    for (size_t idFace = ownStart[idCell]; idFace < ownStart[idCell+1]; idFace++)\
    {\
        expression\
    };
#define forAllNbr(losortStart,losort,idCell,nbrFace,expression)\
    for (size_t idlosort = losortStart[idCell]; idlosort < losortStart[idCell+1]; idlosort++)\
    {\
        const Foam::label nbrFace = losort[idlosort];\
        if(nbrFace != -1){\
            expression\
        };\
    };

#define parallelForAll(size,index,expression)\
    Foam::foamExecutor exec;\
    auto Lambda = [=](label index){expression};\
    exec.parallelFor(Lambda,size);

#endif

