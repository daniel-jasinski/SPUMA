#ifndef Foam_cuda_Atomic_H
#define Foam_cuda_Atomic_H

#include "Atomic.H"
#include "cudaDeviceUtils.cuh"

namespace Foam
{

struct cudaAtomic
: Atomic<cudaAtomic>
{
    template<class T>
    struct atomicPlusEqOp
    {
        void operator()(T& x, const T& y) const
        {
            atomicAdd(&x,y);
        };
    };

    static void  _backendAtomicAdd(solveScalar& x, const solveScalar& y)
    {
        atomicAdd(&x,y);
    };

    static void  _backendAtomicAdd(label& x, const label& y)
    {
        atomicAdd(&x,y);
    };

    static void  _backendAtomicMax(solveScalar& x, const solveScalar& y)
    {
        atomicMax(&x,y);
    };

    static void  _backendAtomicMax(label& x, const label& y)
    {
        atomicMax(&x,y);
    };

    template<class T>
    struct atomicMaxEqOp
    {
        void operator()(T& x, const T& y) const
        {
            _backendAtomicMax(x,y);
        };
    };

    static void  _backendAtomicMin(solveScalar& x, const solveScalar& y)
    {
        atomicMin(&x,y);
    };

    static void  _backendAtomicMin(label& x, const label& y)
    {
        atomicMin(&x,y);
    };

    template<class T>
    struct atomicMinEqOp
    {
        void operator()(T& x, const T& y) const
        {
            _backendAtomicMin(x,y);
        };
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts,0>::eqOp(vs1, vs2, atomicPlusEqOp<Cmpt>());
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMax
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts,0>::eqOp(vs1, vs2, atomicMaxEqOp<Cmpt>());
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMin
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts,0>::eqOp(vs1, vs2, atomicMinEqOp<Cmpt>());
    };

    static label _backendAtomicCAS(label& x,const label& compare, const label& y)
    {
        return atomicCAS(&x,compare,y);
    };

};

} // namespace Foam

#endif
