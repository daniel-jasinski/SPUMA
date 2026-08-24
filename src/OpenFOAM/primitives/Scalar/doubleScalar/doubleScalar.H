/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2017-2020 OpenCFD Ltd.
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM.

    OpenFOAM is free software: you can redistribute it and/or modify it
    under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    OpenFOAM is distributed in the hope that it will be useful, but WITHOUT
    ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
    FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with OpenFOAM.  If not, see <http://www.gnu.org/licenses/>.

Typedef
    Foam::doubleScalar

Description
    Floating-point double precision scalar type.

SourceFiles
    doubleScalar.cxx

\*---------------------------------------------------------------------------*/

#ifndef Foam_primitives_double_H
#define Foam_primitives_double_H

#include "scalarFwd.H"
#include "doubleFloat.H"
#include "direction.H"
#include "pTraits.H"
#include "word.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Typedef (doubleScalar) in scalarFwd.H

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Largest and smallest scalar values allowed in certain parts of the code.
// See std::numeric_limits max(), min(), epsilon()
constexpr doubleScalar doubleScalarGREAT = 1.0e+15;
constexpr doubleScalar doubleScalarROOTGREAT = 3.0e+8;
constexpr doubleScalar doubleScalarVGREAT = 1.0e+300;
constexpr doubleScalar doubleScalarROOTVGREAT = 1.0e+150;
constexpr doubleScalar doubleScalarSMALL = 1.0e-15;
constexpr doubleScalar doubleScalarROOTSMALL = 3.0e-8;
constexpr doubleScalar doubleScalarVSMALL = 1.0e-300;
constexpr doubleScalar doubleScalarROOTVSMALL = 1.0e-150;


#define Scalar doubleScalar
#define ScalarVGREAT doubleScalarVGREAT
#define ScalarVSMALL doubleScalarVSMALL
#define ScalarROOTVGREAT doubleScalarROOTVGREAT
#define ScalarROOTVSMALL doubleScalarROOTVSMALL
#define ScalarRead readDouble


// When compiling SYCL code for CUDA device (via AdaptiveCpp), clang converts
// ::sin(double) etc. to LLVM fsin/fcos intrinsics before AdaptiveCpp's LLVM
// passes can intercept them. The NVPTX backend cannot lower f64 intrinsics.
// Fix: call NVIDIA's device math library (__nv_sin etc.) directly on device.
#if defined(__CUDA_ARCH__)

// NVIDIA device math library declarations (defined in libdevice bitcode)
extern "C" __device__ double __nv_fabs(double);
extern "C" __device__ double __nv_hypot(double, double);
extern "C" __device__ double __nv_atan2(double, double);
extern "C" __device__ double __nv_sqrt(double);
extern "C" __device__ double __nv_cbrt(double);
extern "C" __device__ double __nv_exp(double);
extern "C" __device__ double __nv_log(double);
extern "C" __device__ double __nv_log10(double);
extern "C" __device__ double __nv_sin(double);
extern "C" __device__ double __nv_cos(double);
extern "C" __device__ double __nv_tan(double);
extern "C" __device__ double __nv_asin(double);
extern "C" __device__ double __nv_acos(double);
extern "C" __device__ double __nv_atan(double);
extern "C" __device__ double __nv_sinh(double);
extern "C" __device__ double __nv_cosh(double);
extern "C" __device__ double __nv_tanh(double);
extern "C" __device__ double __nv_asinh(double);
extern "C" __device__ double __nv_acosh(double);
extern "C" __device__ double __nv_atanh(double);
extern "C" __device__ double __nv_erf(double);
extern "C" __device__ double __nv_erfc(double);
extern "C" __device__ double __nv_lgamma(double);
extern "C" __device__ double __nv_tgamma(double);

FOAM_DEVICE inline Scalar mag(const Scalar s)
{
    return __nv_fabs(s);
}

FOAM_DEVICE inline Scalar hypot(const Scalar x, const Scalar y)
{
    return __nv_hypot(x, y);
}

FOAM_DEVICE inline Scalar atan2(const Scalar y, const Scalar x)
{
    return __nv_atan2(y, x);
}

// Double-precision transcendental functions via NVIDIA device math
#define transFunc(func)                                         \
FOAM_DEVICE inline Scalar func(const Scalar s)                  \
{                                                               \
    return __nv_##func(s);                                      \
}

#else /* Host compilation */

FOAM_DEVICE inline Scalar mag(const Scalar s)
{
    return ::fabs(s);
}

FOAM_DEVICE inline Scalar hypot(const Scalar x, const Scalar y)
{
    return ::hypot(x, y);
}

FOAM_DEVICE inline Scalar atan2(const Scalar y, const Scalar x)
{
    return ::atan2(y, x);
}

// Normal (double-precision) transcendental functions
#define transFunc(func)                                         \
inline Scalar func(const Scalar s)                              \
{                                                               \
    return ::func(s);                                           \
}

#endif /* __CUDA_ARCH__ */

// Normal (double-precision) bessel functions.
// May not be available on all systems
#if defined(__CUDA_ARCH__)
    // CUDA device: use NVIDIA device math library for Bessel functions
    extern "C" __device__ double __nv_j0(double);
    extern "C" __device__ double __nv_j1(double);
    extern "C" __device__ double __nv_y0(double);
    extern "C" __device__ double __nv_y1(double);
    extern "C" __device__ double __nv_jn(int, double);
    extern "C" __device__ double __nv_yn(int, double);
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return __nv_##func(s);                                  \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return __nv_##func(n, s);                               \
    }
#elif defined(Foam_no_besselFunc)
    // Not available
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        std::cerr<< "No '" << #func << "' function\n";          \
        return 0;                                               \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        std::cerr<< "No '" << #func << "' function\n";          \
        return 0;                                               \
    }
#elif defined(__MINGW32__)
    // Mingw: with '_' prefix
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return _##func(s);                                      \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return _##func(n, s);                                   \
    }
#elif defined(_MSC_VER)
    // MSVC ABI (incl. Clang targeting MSVC): '_' prefix
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return _##func(s);                                      \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return _##func(n, s);                                   \
    }
#else
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return ::func(s);                                       \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return ::func(n, s);                                    \
    }
#endif


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// scalarImpl.H is used for template-like substitution (but using macros)

#define Foam_use_scalarImpl_header
#include "scalarImpl.H"
#undef Foam_use_scalarImpl_header

#undef Scalar
#undef ScalarVGREAT
#undef ScalarVSMALL
#undef ScalarROOTVGREAT
#undef ScalarROOTVSMALL
#undef ScalarRead
#undef transFunc
#undef besselFunc
#undef besselFunc2


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
