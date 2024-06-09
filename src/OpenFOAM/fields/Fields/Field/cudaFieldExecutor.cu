
#include "cudaFieldExecutor.H"

#ifdef have_cuda
    #include <cuda_runtime_api.h>
    #define deviceSync() cudaDeviceSynchronize()
#else 
    #define deviceSync() 
#endif

#define NUM_THREADS_PER_BLOCK 128
#define SET_NUM_BLOCKS(size)           \
    (size + NUM_THREADS_PER_BLOCK - 1) \
    / NUM_THREADS_PER_BLOCK


namespace Foam
{
//kernels
namespace cuda
{

template <typename cmptType, direction nComponents> //cmpType : int, double , complex:( etc
__global__
void negate(cmptType* f1p, const label loop_len)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len*nComponents)
        f1p[i] = - f1p[i]; // assume that - operator is defined for Type // note problem for complex

};

}; // end namespace cuda

};

// Memeber functions

template<typename Type>
void Foam::cudaFieldExecutor<Type>::negate(Type* fieldPtr, const label loop_len)
{
    //typedef typename pTraits<Type>::cmptType cmptType;
    //static constexpr direction nComponents = pTraits_nComponents<Type>::value;
    
    cmptType* __restrict__ f1p = reinterpret_cast<cmptType*>(fieldPtr); // array continuos in memory
    //Type* __restrict__ f1p = fieldPtr; // array continuos in memory
    
    const label numBlocks = SET_NUM_BLOCKS(loop_len*nComponents);

    Foam::cuda::negate<cmptType,nComponents><<<numBlocks, NUM_THREADS_PER_BLOCK>>>(f1p, loop_len);
    Info << "EXEC: ciao negate" << endl;

    deviceSync();
};


// template specialization
template class Foam::cudaFieldExecutor<Foam::scalar>; 
template class Foam::cudaFieldExecutor<Foam::vector>; 
template class Foam::cudaFieldExecutor<Foam::tensor>; 
//template<> void Foam::cudaFieldExecutor<Foam::scalar>::negate(scalar* fieldPtr, const label loop_len);
//template<> void Foam::cudaFieldExecutor<Foam::vector>::negate(vector* fieldPtr, const label loop_len);
//template void Foam::cudaFieldExecutor::negate<Foam::scalar>(Foam::scalar* fieldPtr, const Foam::label loop_len);
