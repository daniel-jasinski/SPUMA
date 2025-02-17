#ifndef Foam_Atomic_H
#define Foam_Atomic_H

#include "scalar.H"
#include "label.H"
#include "VectorSpace.H"


namespace Foam
{

template <typename T>
struct Atomic
{
    template<typename Type>
    FOAM_DEVICE static void AtomicAdd(Type& x,const Type& y)
    {
        T::_backendAtomicAdd(x,y);
    };

    template<class Form, class Cmpt, direction Ncmpts>
    FOAM_DEVICE static void AtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        T::_backendAtomicAdd(vs1,vs2);
    };

    template<typename Type>
    FOAM_DEVICE static void AtomicMax(Type& x, const Type& y)
    {
        T::_backendAtomicMax(x,y);
    }

    template<class Form, class Cmpt, direction Ncmpts>
    FOAM_DEVICE static void AtomicMax
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        T::_backendAtomicMax(vs1,vs2);
    };

    template<typename Type>
    FOAM_DEVICE static void AtomicMin(Type& x, const Type& y)
    {
        T::_backendAtomicMin(x,y);
    }

    template<class Form, class Cmpt, direction Ncmpts>
    FOAM_DEVICE static void AtomicMin
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        T::_backendAtomicMin(vs1,vs2);
    };

    FOAM_DEVICE static label AtomicCAS(label& x,const label& compare, const label& y)
    {
        return T::_backendAtomicCAS(x,compare,y);
    };

};

} // namespace Foam

#endif
