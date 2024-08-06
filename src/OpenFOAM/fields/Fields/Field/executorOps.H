#ifndef Foam_executor_ops_H
#define Foam_executor_ops_H

/*  use cmptType and Type to properly cast input pointer and set nComponents
    accordingly to launch size*ncompontes when possible
    further typdef are required in this implementation to allow the executors
    to be templated just by the op.
*/
#include "deviceM.H"
#include "pTraits.H"
#include "scalar.H"
#include <type_traits>

namespace Foam
{

namespace exec
{

//define recursive compile time func to define other types?

// handles base types
template<class T,class T1, class T2, class T3>
struct baseTypes
{
    using execResultT = T;
    using execT1 = T1;
    using execT2 = T2;
    using execT3 = T3;

    using resultT = T;
    using Type1 = T1;
    using Type2 = T2;
    using Type3 = T3;

    static constexpr direction nComponents = 1;
};

template<class T,class T1, class T2, class T3>
struct cmptTypes
{
    using execResultT = T;
    using execT1 = T1;
    using execT2 = T2;
    using execT3 = T3;

    using resultT = typename pTraits<T>::cmptType;
    using Type1 = typename pTraits<T1>::cmptType;
    using Type2 = typename pTraits<T2>::cmptType;
    using Type3 = typename pTraits<T3>::cmptType;

    static constexpr direction nComponents = pTraits_nComponents<T>::value;
};

typedef baseTypes<scalar,scalar,scalar,scalar> scalarTypes;

//
template<typename T,typename T1>
struct assignOp
: cmptTypes<T,T1,T1,T1>
{

    typedef cmptTypes<T,T1,T1,T1> BTypes;
    FOAM_DEVICE_I void operator()
    (
        typename BTypes::resultT& z, 
        const typename BTypes::Type1& x
    ) const
    {
        z = x;
    }; 
};

//NOTE on void ops: arguments must be passed by reference
template<typename T,typename T1, typename T2>
struct noopOp3
: baseTypes<T,T1,T2,T2>
{   
    typedef baseTypes<T,T1,T1,T1> BTypes;
    FOAM_DEVICE_I void operator()
    (
        typename BTypes::resultT& z,
        const typename BTypes::Type1& x,
        const typename BTypes::Type1& y
    ) {};
};

#define binaryOp(opName, TYPE, op)             \
template <typename T, typename T1>             \
struct opName##Op2                             \
: public TYPE##Types<T, T1, T1, T1>            \
{                                              \
    typedef TYPE##Types<T, T1, T1, T1> BTypes; \
    FOAM_DEVICE_I void operator()              \
    (                                          \
        typename BTypes::resultT &x,           \
        const typename BTypes::Type1 &y        \
    ) const                                    \
    {                                          \
        op;                                    \
    }                                          \
};

binaryOp(eqSum, cmpt, x += y)
binaryOp(eqMinus, cmpt, x -= y)
binaryOp(eqProd, base, x *= y)
binaryOp(eqDivide, base, x /= y)

#undef binaryOp

#define binaryOp(opName, op)             \
template <typename T, typename T1>             \
struct opName##Op                             \
: public baseTypes<T, T1, T1, T1>            \
{                                              \
    typedef baseTypes<T, T1, T1, T1> BTypes; \
    FOAM_DEVICE_I void operator()              \
    (                                          \
        typename BTypes::resultT &x,           \
        const typename BTypes::Type1 &y        \
    ) const                                    \
    {                                          \
        op;                                    \
    }                                          \
};

binaryOp(mag, x = mag(y))
binaryOp(dev2, x = dev(y))
binaryOp(dev, x = dev2(y))
binaryOp(tr, x = tr(y))
binaryOp(cof, x = cof(y))
binaryOp(det, x = det(y))
binaryOp(symm, x = symm(y))
binaryOp(sph, x = sph(y))
binaryOp(skew, x = skew(y))

// specialization of particular ops
template <typename T, typename T1>
struct negateOp2
: public cmptTypes<T,T1,T1,T1>
{
    typedef cmptTypes<T,T1,T1,T1> BTypes;
    FOAM_DEVICE_I void operator()
    (
        typename BTypes::resultT& x,
        const typename BTypes::Type1& y
    ) const
    {
        x = -y;
    }
};

template <typename T, typename T1>
struct hdualOp2
: public baseTypes<T,T1,T1,T1>
{

    typedef baseTypes<T,T1,T1,T1> BTypes;
    FOAM_DEVICE_I void operator()
    (
        typename BTypes::resultT& x,
        const typename BTypes::Type1& y
    ) const
    {
        x = *y;
    }
};

template <typename T, typename T1>
struct componentOp
: public baseTypes<T,T1,T1,T1>
{
    typedef baseTypes<T,T1,T1,T1> BTypes;
    explicit componentOp(const direction d) noexcept: c(d) {};

    FOAM_DEVICE_I void operator()
    (
        typename BTypes::resultT &x,
        const typename BTypes::Type1 &y
    ) const
    {
        x = y.component(c);
    }

private:
   direction c;
};

template <typename T,typename T1>
struct replaceOp
: public baseTypes<T,T1,T1,T1>
{
    typedef baseTypes<T,T1,T1,T1> BTypes;
    explicit replaceOp(const direction d) noexcept: c(d) {};
    
    FOAM_DEVICE_I void operator()
    (
        typename BTypes::resultT& x,
        const typename  BTypes::Type1& y
    ) const
    {
        x.replace(c,y);
    }

private:
    direction c;
};

template<class T,class T1, class T2, class T3>
struct lerpOp
: public baseTypes<T,T1,T2,T3>
{

    typedef baseTypes<T,T1,T2,T3> BTypes; 

    FOAM_DEVICE_I void operator()
    (
        typename BTypes::resultT& z,
        const typename BTypes::Type1& x,
        const typename BTypes::Type1& y,
        const typename BTypes::Type3& scaling
    ) const
    {
        z = lerp(x,y,scaling);
    };

};

// Ops involing 3 arguments
#define Op(opName, op)                          \
template <typename T, typename T1, typename T2> \
struct opName##Op3                              \
: public baseTypes<T,T1,T2,T2>                  \
{                                               \
    typedef baseTypes<T,T1,T2,T2> BTypes;       \
    FOAM_DEVICE_I void operator()               \
    (                                           \
        typename BTypes::resultT& z,            \
        const typename BTypes::Type1& x,        \
        const typename BTypes::Type2& y         \
    ) const                                     \
    {                                           \
        op;                                     \
    }                                           \
};

//
Op(add, z = x + y);
Op(subtract, z = x - y)
Op(multiply, z = x * y)
Op(divide, z = x / y)

Op(outer, z = x * y)
Op(cross, z = x ^ y)
Op(dot, z = x & y)
Op(dotdot, z = x && y)


// partial specialization when the type is the same for all and ops are between fields
#define OpSameType(opName, op)           \
template <typename T>                    \
struct opName##Op3<T, T, T>              \
: public cmptTypes<T,T,T,T>              \
{                                        \
    typedef cmptTypes<T,T,T,T> BTypes;   \
    FOAM_DEVICE_I void operator()        \
    (                                    \
        typename BTypes::resultT& z,     \
        const typename BTypes::Type1& x, \
        const typename BTypes::Type2& y  \
    ) const                              \
    {                                    \
        op;                              \
    }                                    \
};

OpSameType(add, z = x + y)
OpSameType(subtract, z = x - y)

// partial specialization for  1 scalar argument // adjust instead of scalar typename pTraiTs<T>::cmptType
/*
#define OpScalar(opName, op)                  \
template <typename T, typename T2>            \
struct opName##Op3<T, scalar, T2>             \
: public cmptTypes<T,scalar,T2,T2>            \
{                                             \
    typedef cmptTypes<T,scalar,T2,T2> BTypes; \
    FOAM_DEVICE_I void operator()             \
    (                                         \
        typename BTypes::resultT& z,          \
        const typename BTypes::Type1& x,      \
        const typename BTypes::Type2& y       \
    ) const                                   \
    {                                         \
        op;                                   \
    }                                         \
};                                            \
template <typename T, typename T1>            \
struct opName##Op3<T, T1, scalar>             \
: public cmptTypes<T,T1,scalar,T1>            \
{                                             \
    typedef cmptTypes<T,T1,scalar,T1> BTypes; \
    FOAM_DEVICE_I void operator()             \
    (                                         \
        typename BTypes::resultT& z,          \
        const typename BTypes::Type1& x,      \
        const typename BTypes::Type2& y       \
    ) const                                   \
    {                                         \
        op;                                   \
    }                                         \
};

OpScalar(add, z = x + y)
OpScalar(subtract, z = x - y)
OpScalar(multiply, z = x * y)
OpScalar(divide, z = x / y)
*/
// full spec for all scalar
#define OpAllScalar(opName, op)                            \
template <>                                                \
struct opName##Op3<scalar, scalar, scalar>                 \
: public baseTypes<scalar,scalar,scalar,scalar>            \
{                                                          \
    typedef baseTypes<scalar,scalar,scalar,scalar> BTypes; \
    FOAM_DEVICE_I void operator()                          \
    (                                                      \
        typename BTypes::resultT& z,                       \
        const typename BTypes::Type1& x,                   \
        const typename BTypes::Type2& y                    \
    )  const                                               \
    {                                                      \
        op;                                                \
    }                                                      \
};

OpAllScalar(add, z = x + y)
OpAllScalar(subtract, z = x - y)
OpAllScalar(multiply, z = x * y)
OpAllScalar(divide, z = x / y)

#undef Op
#undef OpScalar
#undef OpAllScalar
#undef OpSameType


// reduction ops: sum, sumProd, sumSqr, sumMag
//NOTE for sumProd resultType is deduced from cmptType

template<typename T,typename T1, typename T2, class Enable = void>
struct sumProdOp
: public baseTypes<T,T1,T2,T2>
{
    typedef baseTypes<T,T1,T2,T2> BTypes;
    typedef typename BTypes::resultT resultT;
    FOAM_DEVICE_I resultT operator()
    (
        const typename BTypes::Type1& x,
        const typename BTypes::Type2& y
    ) const
    {
        return x&&y;
    };

};

// SFINAE: choose more specialized template
// enable_if has member type if value is true
template<typename T,typename T1,typename T2>
struct sumProdOp<T,T1,T2, typename std::enable_if<std::is_same<T1,T2>::value>::type>
: public cmptTypes<T,T1,T2,T2>
{
    typedef cmptTypes<T,T1,T2,T2> BTypes; 
    typedef typename BTypes::resultT resultT;
    //override ncomponents
    static constexpr direction nComponents = pTraits_nComponents<T1>::value;
    FOAM_DEVICE_I resultT operator()
    (
        const typename BTypes::Type1& x,
        const typename BTypes::Type2& y
    ) const 
    {
        return x*y;
    };
 
};

//- partial specialization to handle float sumProd : link error on float atomicAdd: use double
template<>
struct sumProdOp<double,floatScalar,floatScalar>
: public baseTypes<double,floatScalar,floatScalar,floatScalar>
{
    typedef baseTypes<double,floatScalar,floatScalar,floatScalar> BTypes;
    typedef typename BTypes::resultT resultT;

    FOAM_DEVICE_I resultT operator()
    (
        const typename BTypes::Type1& x,
        const typename BTypes::Type2& y
    ) const
    {
        return x*y;
    };
 
};

#define reductionOp(name, op)                 \
template <typename T, typename T1>            \
struct name##Op                               \
: public baseTypes<T, T1, T1, T1>             \
{                                             \
    typedef baseTypes<T, T1, T1, T1> BTypes;  \
    typedef typename BTypes::resultT resultT; \
    FOAM_DEVICE_I resultT operator()          \
    (                                         \
        const typename BTypes::Type1 &x       \
    ) const                                   \
    {                                         \
        return op;                            \
    };                                        \
};

reductionOp(sum, x)
reductionOp(sumSqr, sqr(x))
reductionOp(sumMag, mag(x))
#undef reductionOp

// scalar ops
#define scalarOp(name, op)                             \
template <typename T, typename T1>                     \
struct name##Op                                        \
{                                                      \
    name##Op(){NotImplemented;}                        \
};                                                     \
template <>                                            \
struct name##Op<scalar, scalar>                        \
    : public scalarTypes                               \
{                                                      \
    typedef scalarTypes BTypes;                        \
    FOAM_DEVICE_I void operator()(                     \
        typename BTypes::resultT &x,                   \
        const typename BTypes::Type1 &y) const         \
    {                                                  \
        op;                                            \
    }                                                  \
};

scalarOp(neg, x = neg(y))

#undef scalarOp
}; // end namespace exec

}


#endif
