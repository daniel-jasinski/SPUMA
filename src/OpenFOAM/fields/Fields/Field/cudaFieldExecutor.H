#ifndef Foam_Cuda_Field_Executor_H
#define Foam_Cuda_Field_Executor_H

#include "FieldExecutor.H"

namespace Foam
{

//fwd declaration
template<typename Type> class FieldExecutor;


template <typename Type>
class cudaFieldExecutor
:
    public FieldExecutor<Type>
{
    
    typedef typename pTraits<Type>::cmptType cmptType;
    static constexpr direction nComponents = pTraits_nComponents<Type>::value;
public:
    //template<typename Type>    
    void negate(Type* fieltPtr, const label loop_len);
};

}

//#ifdef NoRepository
//   #include "cudaFieldExecutor.cu"
//#endif

#endif
