#ifndef Foam_Atomic_H
#define Foam_Atomic_H

#include "scalar.H"
#include "label.H"
#include "VectorSpace.H"
#include "error.H"
#include "deviceUtils.H"

namespace Foam
{

template <typename T>
struct Atomic
{
    template<typename Type>
    static void AtomicAdd(Type& x,const Type& y)
    {
        T::_backendAtomicAdd(x,y);
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void AtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        T::_backendAtomicAdd(vs1,vs2);
    };

    template<typename Type>
    static void AtomicMax(Type& x, const Type& y)
    {
        T::_backendAtomicMax(x,y);
    }

    template<class Form, class Cmpt, direction Ncmpts>
    static void AtomicMax
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        T::_backendAtomicMax(vs1,vs2);
    };

    template<typename Type>
    static void AtomicMin(Type& x, const Type& y)
    {
        T::_backendAtomicMin(x,y);
    }

    template<class Form, class Cmpt, direction Ncmpts>
    static void AtomicMin
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        T::_backendAtomicMin(vs1,vs2);
    };

    static label AtomicCAS(label& x,const label& compare, const label& y)
    {
        return T::_backendAtomicCAS(x,compare,y);
    };

};

//dummy host atomics
struct cpuAtomic
: Atomic<cpuAtomic>
{
    template<typename T>
    static void  _backendAtomicAdd(T& x, const T& y)
    {
        //serial case for now
        x += y;
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        vs1 += vs2;
    };

    template<typename T>
    static void  _backendAtomicMax(T& x, const T& y)
    {
        //serial case for now
        x = max(x,y);
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMax
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        vs1 = max(vs1,vs2);
    };

    template<typename T>
    static void  _backendAtomicMin(T& x, const T& y)
    {
        //serial case for now
        x = min(x,y);
    };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMin
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        vs1 = min(vs1,vs2);
    };

    static label _backendAtomicCAS(label& x,const label& compare, const label& y)
    {
        x = (x == compare ? y : x);
        return x;
    };

};

#ifdef have_cuda
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
#endif

} // namespace Foam
#endif

