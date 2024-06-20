#ifndef Foam_executor_ops_H
#define Foam_executor_ops_H

/*  use cmptType and Type do properly cast input pointer and set nComponents
    accordingly to launch size*ncompontes when possible
    further typdef are required in this implementation to allow the executors
    to be template just by the op.
*/
#include "deviceM.H"
#include "pTraits.H"
#include "scalar.H"
#include <type_traits>

namespace Foam
{

namespace exec
{
   
//NOTE on void ops: arguments must be passed by reference
template<typename T,typename T1, typename T2>
struct noopOp3
{   
    typedef T1 execT1;
    typedef T2 execT2;
    typedef T execResultT;
    typedef T1 Type1;
    typedef T2 Type2;
    typedef T resultT;
    static constexpr direction nComponents = 1;
    FOAM_DEVICE_I void operator()(resultT& z, Type1& x, Type2& y){};
};

// non tested macro
/*#define binaryOp(opName, op)                                                 \
template <typename T1, typename T2>                                          \
struct opName##Op2F_OP_F                                                     \
{                                                                            \
    typedef typename pTraits<T1>::cmptType resultT;                          \
    typedef typename pTraits<T2>::cmptType Type;                             \
    static constexpr direction nComponents = pTraits_nComponents<T1>::value; \
    FOAM_DEVICE_I void operator()(resultT& x, const Type& y)                 \
    {                                                                        \
        op;                                                                  \
    }                                                                        \
};                                                                           \
template <typename T1, typename T2>                                          \
struct opName##Op2                                                           \
{                                                                            \
    typedef T1 resultT;                                                      \
    typedef T2 Type;                                                         \
    static constexpr direction nComponents = 1;                              \
    FOAM_DEVICE_I void operator()(resultT& x, const Type& y)                 \
    {                                                                        \
        op;                                                                  \
    }                                                                        \
};*/

// specialization of particular ops
template <typename T, typename T1>
struct negateOp2
{
    typedef T1 execT1;
    typedef T1 execT2; // treat this as void?
    typedef T execResultT;
    typedef typename pTraits<T>::cmptType resultT;
    typedef typename pTraits<T1>::cmptType Type;
    static constexpr direction nComponents = pTraits_nComponents<T>::value;
    FOAM_DEVICE_I void operator()(resultT& x, const Type& y)
    {
        x = -y;
    }
};

template <typename T, typename T1>
struct hdualOp2
{
    typedef T1 execT1;
    typedef T1 execT2; // treat this as void?
    typedef T execResultT;

    typedef T resultT;
    typedef T1 Type;
    static constexpr direction nComponents = 1;
    FOAM_DEVICE_I void operator()(resultT& x, const Type& y)
    {
        x = *y;
    }
};

// Triple template parameter
#define Op(opName, op)                                                        \
template <typename T, typename T1, typename T2>                               \
struct opName##Op3F_OP_F                                                      \
{                                                                             \
    typedef T1 execT1;                                                        \
    typedef T2 execT2;                                                        \
    typedef T execResultT;                                                    \
    typedef T1 Type1;                                                         \
    typedef T2 Type2;                                                         \
    typedef T resultT;                                                        \
    static constexpr direction nComponents = 1;                               \
    FOAM_DEVICE_I void operator()(resultT &z, const Type1 &x, const Type2 &y) \
    {                                                                         \
        op;                                                                   \
    }                                                                         \
};                                                                            \
template <typename T, typename T1, typename T2>                               \
struct opName##Op3                                                            \
{                                                                             \
    typedef T1 execT1;                                                        \
    typedef T2 execT2;                                                        \
    typedef T execResultT;                                                    \
    typedef T1 Type1;                                                         \
    typedef T2 Type2;                                                         \
    typedef T resultT;                                                        \
    static constexpr direction nComponents = 1;                               \
    FOAM_DEVICE_I void operator()(resultT &z, const Type1 &x, const Type2 &y) \
    {                                                                         \
        op;                                                                   \
    }                                                                         \
};

Op(add, z = x + y);
Op(subtract, z = x - y)
Op(minus, z = x - y)
Op(multiply, z = x * y)
Op(divide, z = x / y)

Op(outer, z = x * y)
Op(cross, z = x ^ y)
Op(dot, z = x & y)
Op(dotdot, z = x && y)


// partial specialization when the type is the same for all and ops are between fields
#define OpHomo(opName, op)                                                    \
template <typename T>                                                         \
struct opName##Op3F_OP_F<T, T, T>                                             \
{                                                                             \
    typedef T execT1;                                                         \
    typedef T execT2;                                                         \
    typedef T execResultT;                                                    \
    typedef typename pTraits<T>::cmptType Type1;                              \
    typedef typename pTraits<T>::cmptType Type2;                              \
    typedef typename pTraits<T>::cmptType resultT;                            \
    static constexpr direction nComponents = pTraits_nComponents<T>::value;   \
    FOAM_DEVICE_I void operator()(resultT &z, const Type1 &x, const Type2 &y) \
    {                                                                         \
        op;                                                                   \
    }                                                                         \
};

                                        OpHomo(add, z = x + y)
                                            OpHomo(subtract, z = x - y)
                                                OpHomo(minus, z = x - y)

// partial specialization for  scalar
#define OpScalar(opName, op)                                                   \
template <typename T, typename T2>                                             \
struct opName##Op3F_OP_F<T, scalar, T2>                                        \
{                                                                              \
    typedef scalar execT1; \
    typedef T2 execT2; \
    typedef T execResultT; \
    typedef scalar Type1;                                                      \
    typedef T2 Type2;                                                          \
    typedef T resultT;                                                         \
    static constexpr direction nComponents = 1;                                \
    FOAM_DEVICE_I void operator()(resultT &z, const Type1 &x, const Type2 &y)  \
    {                                                                          \
        op;                                                                    \
    }                                                                          \
};                                                                             \
template <typename T, typename T1>                                             \
struct opName##Op3F_OP_F<T, T1, scalar>                                        \
{                                                                              \
    typedef T1 execT1; \
    typedef scalar execT2; \
    typedef T execResultT; \
    typedef T1 Type1;                                                          \
    typedef scalar Type2;                                                      \
    typedef T resultT;                                                         \
    static constexpr direction nComponents = 1;                                \
    FOAM_DEVICE_I void operator()(resultT &z, const Type1 &x, const scalar &y) \
    {                                                                          \
        op;                                                                    \
    }                                                                          \
};                                                                             \
template <typename T, typename T2>                                             \
struct opName##Op3<T, scalar, T2>                                              \
{                                                                              \
    typedef scalar execT1; \
    typedef T2 execT2; \
    typedef T execResultT; \
    typedef scalar Type1;                                                      \
    typedef typename pTraits<T2>::cmptType Type2;                              \
    typedef typename pTraits<T>::cmptType resultT;                             \
    static constexpr direction nComponents = pTraits_nComponents<T>::value;    \
    FOAM_DEVICE_I void operator()(resultT &z, const scalar &x, const Type2 &y) \
    {                                                                          \
        op;                                                                    \
    }                                                                          \
};                                                                             \
template <typename T, typename T1>                                             \
struct opName##Op3<T, T1, scalar>                                              \
{                                                                              \
    typedef T1 execT1; \
    typedef scalar execT2; \
    typedef T execResultT; \
    typedef typename pTraits<T1>::cmptType Type1;                              \
    typedef scalar Type2;                                                      \
    typedef typename pTraits<T>::cmptType resultT;                             \
    static constexpr direction nComponents = pTraits_nComponents<T>::value;    \
    FOAM_DEVICE_I void operator()(resultT &z, const Type1 &x, const scalar &y) \
    {                                                                          \
        op;                                                                    \
    }                                                                          \
};

OpScalar(add, z = x + y)
OpScalar(subtract, z = x - y)
OpScalar(minus, z = x - y)
OpScalar(multiply, z = x * y)
OpScalar(divide, z = x / y)

// full spec for all scalar
#define OpAllScalar(opName, op)                                                \
template <>                                                                    \
struct opName##Op3F_OP_F<scalar, scalar, scalar>                               \
{                                                                              \
    typedef scalar execT1;                                                     \
    typedef scalar execT2;                                                     \
    typedef scalar execResultT;                                                \
    typedef scalar Type1;                                                      \
    typedef scalar Type2;                                                      \
    typedef scalar resultT;                                                    \
    static constexpr direction nComponents = 1;                                \
    FOAM_DEVICE_I void operator()(scalar &z, const scalar &x, const scalar &y) \
    {                                                                          \
        op;                                                                    \
    }                                                                          \
};                                                                             \
template <>                                                                    \
struct opName##Op3<scalar, scalar, scalar>                                     \
{                                                                              \
    typedef scalar execT1;                                                     \
    typedef scalar execT2;                                                     \
    typedef scalar execResultT;                                                \
    typedef scalar Type1;                                                      \
    typedef scalar Type2;                                                      \
    typedef scalar resultT;                                                    \
    static constexpr direction nComponents = 1;                                \
    FOAM_DEVICE_I void operator()(scalar &z, const scalar &x, const scalar &y) \
    {                                                                          \
        op;                                                                    \
    }                                                                          \
};

OpAllScalar(add, z = x + y)
OpAllScalar(subtract, z = x - y)
OpAllScalar(minus, z = x - y)
OpAllScalar(multiply, z = x * y)
OpAllScalar(divide, z = x / y)

#undef Op
#undef OpCmpt
#undef OpScalar
#undef OpAllScalar
#undef OpHomo


}; // end namespace exec

}


#endif
