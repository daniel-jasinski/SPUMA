#ifndef Foam_Field_Executor_H
#define Foam_Field_Executor_H

#include "label.H"
#include "scalar.H"
#include "vector.H"
#include "tensor.H"
#include "word.H"

#include "cpuFieldExecutor.H"
#include "cudaFieldExecutor.H"

namespace Foam
{

template <typename Type>
class FieldExecutor
{
//private:
    //FieldExecutor<Type>* execPtr = nullptr;

protected:

public:

    //virtual ~FieldExecutor(){ delete execPtr;};

    virtual void negate(Type* fieltPtr, const label loop_len) = 0;

    /*inline static FieldExecutor<Type>* New(word type)
    {   
        if(!execPtr)
        {
            if(type == "cuda")
            {
                execPtr = new cudaFieldExecutor<Type>();
            }
            else if (type == "cpu")
            {
                execPtr = new cpuFieldExecutor<Type>();
            }
            else
            {
                Info << "wrong executor type" << endl;
            }
        }
        
        return execPtr;
    };
    */

};

}

#endif
