#ifndef Foam_cpu_Atomic_H
#define Foam_cpu_Atomic_H

#include "Atomic.H"

namespace Foam
{

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
        label old = x;
        x = (x == compare ? y : x);
        return old;
    };

};

} // namespace Foam


#endif
