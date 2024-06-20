#ifndef Foam_Cuda_field_Executor_C
#define Foam_Cuda_field_Executor_C

#include "cudaFieldExecutor.cuh"
#include "executorOps.H"
#include "deviceM.H"
#include "cudaError.H"
#include <cuda_runtime_api.h>

namespace Foam
{
//kernels
namespace cuda
{

template <typename cmptType, direction nComponents> //cmpType : int, double , complex:( etc
__global__
void negate(cmptType* const __restrict__ fp, const label loop_len)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len*nComponents)
        fp[i] = - fp[i]; // assume that - operator is defined for Type // note problem for complex

};

template<typename Type1,typename Op>
__global__
void opKernel
(
    Type1* const __restrict__ fp,
    Op& op,
    const label loop_len
)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len)
        op(fp[i],fp[i]);
};
// NOTA: in realtá se in op pass puntatori potrei specificare anche li il modo in cui accedo agli stessi?
// se modifico un poco la macro,
// cosí l'executor consisterebbe di un solo kernel!! (o quasi)
// la strategia migliore non é chiara

template <typename Type1, typename Type2,
typename Op >
__global__
void opKernel
(
    Type1* const __restrict__ f1p,
    const Type2* const __restrict__ f2p,
    Op& op,
    const label loop_len
)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len)
        op(f1p[i],f2p[i]); // use scalar operator
};

template <typename resultType, typename Type1, typename Type2,
typename Op >
__global__
void opKernel
(
    resultType* const __restrict__ resultp,
    const Type1* const __restrict__ f1p,
    const Type2* const __restrict__ f2p,
    Op& op,
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
    const Type1 &s,
    const Type2* const __restrict__ fp,
    Op& op,
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
    const Type2 &s,
    Op& op,
    const label loop_len
)
{   
    label i = blockIdx.x * blockDim.x + threadIdx.x;
    if (i < loop_len)
        op(resultp[i], fp[i], s);
};

}; // end namespace cuda

};

/*---------------- Memeber functions ----------------------------------------------- */

//template<typename Type1, typename Type2>
//void Foam::cudaFieldExecutor<Type1,Type2>::negate(Type1* fieldPtr, const label loop_len)
/*template< typename resultType,typename Type1, typename Type2>
void Foam::cudaFieldExecutor<resultType,Type1,Type2>::negate(Type1* fieldPtr, const label loop_len)
{
    typedef typename pTraits<Type1>::cmptType cmptType1;
    static constexpr direction nComponents1 = pTraits_nComponents<Type1>::value;
    
    cmptType1* const __restrict__ f1p = reinterpret_cast<cmptType1*>(fieldPtr); // array continuos in memory
    
    const label numBlocks = SET_NUM_BLOCKS(loop_len*nComponents1);

    Foam::cuda::negate<cmptType1,nComponents1><<<numBlocks, NUM_THREADS_PER_BLOCK>>>(f1p, loop_len);
    CHECK_LAST_CUDA_ERROR();
    Info << "EXEC: ciao negate" << endl;

    deviceSync();
};
*/

//template<typename resultType, typename Type1, typename Type2>
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

    CHECK_LAST_CUDA_ERROR();
    //Info << "EXEC: ciao cuda op" << endl;

    deviceSync();
};

//template<typename resultType, typename Type1, typename Type2>
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
    typedef typename Op::Type kernelType;
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

    CHECK_LAST_CUDA_ERROR();
    //Info << "EXEC: ciao cuda op" << endl;

    deviceSync();
};

//template<typename resultType, typename Type1, typename Type2>
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

    CHECK_LAST_CUDA_ERROR();
    //Info << "EXEC: ciao cuda op" << endl;

    deviceSync();
};

//template<typename resultType, typename Type1, typename Type2>
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
    
    //prepare single cmpt to be used/transferred on device
    //if constexpr(!std::is_same<refT,scalar>::value || !std::is_same<refT,label>::value)
    //cudaHostRegister(const_cast<refT*>(&cmptref), sizeof(refT), cudaHostRegisterReadOnly);
    
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
    CHECK_LAST_CUDA_ERROR();
    //if constexpr(!std::is_same<refT,scalar>::value || !std::is_same<refT,label>::value)
    //cudaHostUnregister(const_cast<refT*>(&cmptref));
    //Info << "EXEC: ciao cuda op" << endl;

    deviceSync();
};


//template<typename resultType, typename Type1, typename Type2>
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
    // this is a redundant type casting since at the moment ops are not parallelized on multiple components
    // appropriate pointer casting
    typedef typename Op::resultT resultT;
    typedef typename Op::Type1   T1;
    typedef typename Op::Type2   refT;

    resultT* const resultp = reinterpret_cast<resultT*>(resultPtr);  
    const T1* const fp =     reinterpret_cast<const T1*>(field2Ptr);  
    const refT& cmptref =    reinterpret_cast<const refT&>(cmptRef);  
    
    //prepare single cmpt to be used/transferred on device
    //if constexpr(!std::is_same<refT,scalar>::value || !std::is_same<refT,label>::value)
    //cudaHostRegister(const_cast<refT*>(&cmptref), sizeof(refT), cudaHostRegisterReadOnly);
    //TODO:
    /* if (!MemoryPool::getInstance()->isRegistered(&cmptRef))
        MemoryPool::getInstance()->register(&cmptRef)
    */
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
    
    CHECK_LAST_CUDA_ERROR();
    //if constexpr(!std::is_same<refT,scalar>::value || !std::is_same<refT,label>::value)
    //cudaHostUnregister(const_cast<refT*>(&cmptref));
    //Info << "EXEC: ciao cuda op" << endl;

    deviceSync();
};



#endif
