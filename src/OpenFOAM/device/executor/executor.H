#ifndef Foam_executor_H
#define Foam_executor_H

#include "label.H"

#define FOAM_BACKEND cudaExecutor

namespace Foam
{
//fwd declaration
template <typename T> class executor;

template <typename T>
class executor
{

public:
    executor() = default;
    ~executor() = default;
    // No copy constructor
    executor(executor<T> &other) = delete;

    template<typename F> 
    void parallelFor(F& lambda, const label& size){
        (static_cast<T*>(this))->_backendFor(lambda, size);
    };
    
    template<typename F, typename resultT>
    void reductionSum(F& lambda, resultT* result, const label& size){
        (static_cast<T*>(this))->_backendReductionSum(lambda, result, size);
    };

    //operator
    void operator=(const executor<T> &) = delete;
};


} // namespace Foam


#endif
