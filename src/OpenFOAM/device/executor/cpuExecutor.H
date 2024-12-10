#ifndef Foam_cpu_executor_H
#define Foam_cpu_executor_H

#include "executor.H"

namespace Foam
{


//fwd declaration
//template <typename T> class executor;
class cpuExecutor;


class cpuExecutor
: 
    public executor<cpuExecutor>
{

public:
    
    //cpuExecutor() = default;
    
    //~cpuExecutor() = default;


    template<typename F>
    void _backendFor(F& lambda, const label& size)
    {
        for (label i = 0; i < size; i++)
        {
            lambda(i);
        }
    };

    template<typename F>
    void _backendSerialFor(F& lambda, const label& size)
    {
        _backendFor(lambda,size);
    };

    template<typename F, typename resultT>
    void _backendReductionSum(F& lambda, resultT* const __restrict__ result, const label& size)
    {
        for (label i = 0; i < size; i++)
        {
            *result += lambda(i);
        }  
    };

    template<typename F,typename Op, typename resultT>
    void _backendReductionCompare(F& lambda, Op& op, resultT* const __restrict__ result, const label& size)
    {
        for (label i = 0; i < size; i++)
        {
            *result = op(lambda(i),*result);
        }  
    };


};


} // namespace Foam


#endif
