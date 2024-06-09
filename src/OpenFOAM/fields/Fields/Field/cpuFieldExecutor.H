#ifndef Foam_Cpu_Field_Executor_H
#define Foam_Cpu_Field_Executor_H

#include "FieldExecutor.H"

namespace Foam
{

template<typename Type> class FieldExecutor;

template <typename Type>
class cpuFieldExecutor
:
    public FieldExecutor<Type>
{    
public:
    
    //cpuFieldExecutor();
    //~cpuFieldExecutor();

    inline void negate(Type* fieltPtr, const label loop_len)
    {
        Type* const __restrict__ fp = fieltPtr;
        for (label i = 0; i < loop_len; i++)
        {
            fp[i] = - fp[i]; //operator defined for each underlying type i.e vector, scalar, complex, tensor, etc..
        }
        
    };
};

}

#endif
