#ifndef Foam_Cuda_Field_Executor_H
#define Foam_Cuda_Field_Executor_H

#include "FieldExecutor.H"

namespace Foam
{

//fwd declaration

template<typename Op>
class cudaFieldExecutor
:
    public refCount
{
    typedef typename Op::execResultT resultType;
    typedef typename Op::execT1 Type1;
    typedef typename Op::execT2 Type2;
    typedef typename Op::execT3 Type3;

public:
    
    //cudaFieldExecutor() = default;
    //NOTE pass op as rvalue reference
    void opF_OP_F
    (
        resultType* resultPtr,
        Op op,
        const label loop_len
    );

    void opF_OP_F
    (
        resultType* resultPtr,
        const Type1* field1Ptr,
        Op op,
        const label loop_len
    );


    void opF_OP_S
    (
        resultType* resultPtr,
        const Type1& cmptRef,
        Op op,
        const label loop_len
    );
    
    // TODO change Op to allow for move semantic
    void opF_OP_F
    (
        resultType* resultPtr,
        const Type1* field1Ptr,
        const Type2* field2Ptr,
        Op op,
        const label loop_len
    );
    
    void opS_OP_F
    (
        resultType* resultPtr,
        const Type1 &cmptRef,
        const Type2* fieldPtr,
        Op op,
        const label loop_len
    );
    
    void opF_OP_S
    (
        resultType* resultPtr,
        const Type1* field2Ptr,
        const Type2 &cmptRef,
        Op op,
        const label loop_len
    );

    void opF_OP_F_F
    (
        resultType* resultPtr,
        const Type1* field1Ptr,
        const Type2* field2Ptr,
        const Type3* field3Ptr,
        Op op,
        const label loop_len
    );
    
    void opF_OP_F_S
    (
        resultType* resultPtr,
        const Type1* field1Ptr,
        const Type2* field2Ptr,
        const Type3 &cmptRef,
        Op op,
        const label loop_len
    );

    void reductionSum
    (
        resultType &result,
        const Type1* field1Ptr,
        const Type2* field2Ptr,
        Op op,
        const label loop_len 
    );

    void reductionSum
    (
        resultType &result,
        const Type1* field1Ptr,
        Op op,
        const label loop_len 
    );

    void reductionEq
    (
        resultType &result,
        const Type1* field1Ptr,
        Op op,
        const label loop_len 
    );

};

}

#ifdef NoRepository 
    #ifdef have_cuda
    #include "cudaFieldExecutor.cu"
    #endif
#endif

#endif
