#ifndef Foam_Atomic_H
#define Foam_Atomic_H

#include "scalar.H"
#include "label.H"
#include "VectorSpace.H"
#include "error.H"


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
    
    // template<typename T>
    // static 
    // typename std::enable_if<std::is_arithmetic<T>()>::type
    // _backendAtomicAdd(T& x, const T& y)
    // {
    //     atomicAdd(&x,y);   
    // };

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        VectorSpaceOps<Ncmpts,0>::eqOp(vs1, vs2, atomicPlusEqOp<Cmpt>());
    };

    static label _backendAtomicCAS(label& x,const label& compare, const label& y)
    {
        return atomicCAS(&x,compare,y);
    };

};
#endif

} // namespace Foam
#endif

