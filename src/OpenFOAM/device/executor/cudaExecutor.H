#ifndef Foam_cuda_executor_H
#define Foam_cuda_executor_H

#include "executor.H"

namespace Foam
{
//fwd declaration
class cudaExecutor;


class cudaExecutor
: 
    public executor<cudaExecutor>
{

public:

    //cudaExecutor() = default;
    
    //~cudaExecutor() = default;

    template<typename F>
    void _backendFor(F& lambda, const label& size);

    template<typename F>
    void _backendSerialFor(F& lambda, const label& size);

    template<typename F, typename resultT>
    void _backendReductionSum(F& lambda, resultT* const __restrict__ result, const label& size);

    template<typename F,typename Op, typename resultT>
    void _backendReductionCompare(F& lambda, Op& op, resultT* const __restrict__ result, const label& size);
};

} // namespace Foam

#ifdef NoRepository
    #include "cudaExecutor.cu"
#endif

#endif
