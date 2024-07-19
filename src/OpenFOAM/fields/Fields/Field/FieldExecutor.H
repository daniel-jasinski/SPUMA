#ifndef Foam_Field_Executor_H
#define Foam_Field_Executor_H

#include "label.H"

#include "cpuFieldExecutor.H"
#ifdef have_cuda
    #include "cudaFieldExecutor.cuh"
#endif

//TODO implementt factory method: add vitual function and implement static selector
namespace Foam
{

// fwd declarations
#ifdef have_cuda
//template<typename ResultType,typename Type1, typename Type2> class cudaFieldExecutor;
template<typename Op> class cudaFieldExecutor;
#endif
//template<typename ResultType,typename Type1, typename Type2> class cpuFieldExecutor;
template<typename Op> class cpuFieldExecutor;

template <typename Op>
class FieldExecutor
{
//private:
protected:
    static FieldExecutor<Op>* execPtr_;

public:

    virtual ~FieldExecutor(){};

    //no copy constructor
    FieldExecutor(FieldExecutor& other) = delete;
    // add virtual methods

    static FieldExecutor<Op>* New();
 
};

}

#ifdef NoRepository
    #include "FieldExecutor.C"
#endif

#endif
