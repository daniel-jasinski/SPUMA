#ifndef Foam_Field_Executor_C
#define Foam_Field_Executor_C

#include "FieldExecutor.H"

template<typename Op>
Foam::FieldExecutor<Op>* Foam::FieldExecutor<Op>::execPtr_ = nullptr;

template<typename Op>
Foam::FieldExecutor<Op>* Foam::FieldExecutor<Op>::New()
{   
    if(!execPtr_)
    {
        #ifdef have_cuda
        execPtr_ = new cudaFieldExecutor<Op>();
        #else
        execPtr_ = new cpuFieldExecutor<Op>();
        #endif
    }
    
    return execPtr_;
};

#endif
