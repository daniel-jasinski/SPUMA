#ifndef Foam_Cpu_Field_Executor_H
#define Foam_Cpu_Field_Executor_H

#include "FieldExecutor.H"

namespace Foam
{


template<typename Op>
class cpuFieldExecutor
:
    public refCount
    //public FieldExecutor<Type1,Type2>
{
    typedef typename Op::execResultT resultType;
    typedef typename Op::execT1 Type1;
    typedef typename Op::execT2 Type2;

public:
    
    //cpuFieldExecutor();
    //~cpuFieldExecutor();

    inline void opF_OP_F
    (
        resultType* resultPtr,
        Op op,
        const label loop_len
    )
    {
        for (label i = 0; i < loop_len; i++)
        {
            op(resultPtr[i],resultPtr[i]);
        }
    };

    inline void opF_OP_F
    (
        resultType* const __restrict__ resultPtr,
        const Type1* const __restrict__ field1Ptr,
        Op op,
        const label loop_len
    )
    {
        for (label i = 0; i < loop_len; i++)
        {
            op(resultPtr[i],field1Ptr[i]);
        }
    };

    inline void opF_OP_F
    (
        resultType* const __restrict__ resultPtr,
        const Type1* const __restrict__ field1Ptr,
        const Type2* const __restrict__ field2Ptr,
        Op op, //find a way to move op instead of passing it by copy
        const label loop_len
    )
    {
        for (label i = 0; i < loop_len; i++)
        {
            op(resultPtr[i],field1Ptr[i], field2Ptr[i]);
        }
    };

    inline void opS_OP_F
    (
        resultType* const __restrict__ resultPtr,
        const Type1& cmptRef,
        const Type2* const __restrict__ field2Ptr,
        Op op,
        const label loop_len
    )
    {
        for (label i = 0; i < loop_len; i++)
        {
            op(resultPtr[i],cmptRef, field2Ptr[i]);
        }        
    };
    
    inline void opF_OP_S
    (
        typename Op::resultT* const __restrict__ resultPtr,
        const Type1* const __restrict__ field2Ptr,
        const Type2& cmptRef,
        Op op,
        const label loop_len
    )
    {
        for (label i = 0; i < loop_len; i++)
        {
            op(resultPtr[i],field2Ptr[i], cmptRef);
        }        
    };
};

}

#endif
