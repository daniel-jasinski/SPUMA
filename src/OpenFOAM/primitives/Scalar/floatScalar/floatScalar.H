/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2017-2020 OpenCFD Ltd.
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
    Foam::floatScalar

Description
    Floating-point single precision scalar type.

SourceFiles
    floatScalar.cxx

\*---------------------------------------------------------------------------*/

#ifndef Foam_primitives_float_H
#define Foam_primitives_float_H

#include "scalarFwd.H"
#include "doubleFloat.H"
#include "direction.H"
#include "pTraits.H"
#include "word.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Typedef (floatScalar) in scalarFwd.H

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Largest and smallest scalar values allowed in certain parts of the code.
// See std::numeric_limits max(), min(), epsilon()
constexpr floatScalar floatScalarGREAT = 1.0e+6;
constexpr floatScalar floatScalarROOTGREAT = 1.0e+3;
constexpr floatScalar floatScalarVGREAT = 1.0e+37;
constexpr floatScalar floatScalarROOTVGREAT = 1.0e+18;
constexpr floatScalar floatScalarSMALL = 1.0e-6;
constexpr floatScalar floatScalarROOTSMALL = 1.0e-3;
constexpr floatScalar floatScalarVSMALL = 1.0e-37;
constexpr floatScalar floatScalarROOTVSMALL = 1.0e-18;


#define Scalar floatScalar
#define ScalarVGREAT floatScalarVGREAT
#define ScalarVSMALL floatScalarVSMALL
#define ScalarROOTVGREAT floatScalarROOTVGREAT
#define ScalarROOTVSMALL floatScalarROOTVSMALL
#define ScalarRead readFloat


// CUDA device: use __nv_* functions (see doubleScalar.H for explanation)
#if defined(__CUDA_ARCH__)

// NVIDIA device math library declarations (float versions)
extern "C" __device__ float __nv_fabsf(float);
extern "C" __device__ float __nv_hypotf(float, float);
extern "C" __device__ float __nv_atan2f(float, float);
extern "C" __device__ float __nv_sqrtf(float);
extern "C" __device__ float __nv_cbrtf(float);
extern "C" __device__ float __nv_expf(float);
extern "C" __device__ float __nv_logf(float);
extern "C" __device__ float __nv_log10f(float);
extern "C" __device__ float __nv_sinf(float);
extern "C" __device__ float __nv_cosf(float);
extern "C" __device__ float __nv_tanf(float);
extern "C" __device__ float __nv_asinf(float);
extern "C" __device__ float __nv_acosf(float);
extern "C" __device__ float __nv_atanf(float);
extern "C" __device__ float __nv_sinhf(float);
extern "C" __device__ float __nv_coshf(float);
extern "C" __device__ float __nv_tanhf(float);
extern "C" __device__ float __nv_asinhf(float);
extern "C" __device__ float __nv_acoshf(float);
extern "C" __device__ float __nv_atanhf(float);
extern "C" __device__ float __nv_erff(float);
extern "C" __device__ float __nv_erfcf(float);
extern "C" __device__ float __nv_lgammaf(float);
extern "C" __device__ float __nv_tgammaf(float);

FOAM_DEVICE inline Scalar mag(const Scalar s)
{
    return __nv_fabsf(s);
}

FOAM_DEVICE inline Scalar hypot(const Scalar x, const Scalar y)
{
    return __nv_hypotf(x, y);
}

FOAM_DEVICE inline Scalar atan2(const Scalar y, const Scalar x)
{
    return __nv_atan2f(y, x);
}

// Single-precision transcendental functions via NVIDIA device math
#define transFunc(func)                                         \
FOAM_DEVICE inline Scalar func(const Scalar s)                  \
{                                                               \
    return __nv_##func##f(s);                                   \
}

#else /* Host compilation */

FOAM_DEVICE inline Scalar mag(const Scalar s)
{
    return ::fabsf(s);
}

FOAM_DEVICE inline Scalar hypot(const Scalar x, const Scalar y)
{
    return ::hypotf(x, y);
}

FOAM_DEVICE inline Scalar atan2(const Scalar y, const Scalar x)
{
    return ::atan2f(y, x);
}

// Single-precision transcendental functions (with 'f' appended to the name)
#define transFunc(func)                                         \
inline Scalar func(const Scalar s)                              \
{                                                               \
    return ::func##f(s);                                        \
}

#endif /* __CUDA_ARCH__ */


// Single-precision bessel functions.
// May not be available on all systems
#if defined(__CUDA_ARCH__)
    // CUDA device: use NVIDIA device math library for Bessel functions
    extern "C" __device__ float __nv_j0f(float);
    extern "C" __device__ float __nv_j1f(float);
    extern "C" __device__ float __nv_y0f(float);
    extern "C" __device__ float __nv_y1f(float);
    extern "C" __device__ float __nv_jnf(int, float);
    extern "C" __device__ float __nv_ynf(int, float);
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return __nv_##func##f(s);                               \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return __nv_##func##f(n, s);                            \
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
#elif defined(__APPLE__)
    // Darwin: no float version (use a cast)
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return ::func(s);                                       \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return Scalar(::func(n, double(s)));                    \
    }
#elif defined(__MINGW32__)
    // Mingw: with '_' prefix, no float version (use a cast)
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return _##func(s);                                      \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return Scalar(_##func(n, double(s)));                   \
    }
#elif defined(_MSC_VER)
    // MSVC ABI (incl. Clang targeting MSVC): '_' prefix, no float version
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return Scalar(_##func(double(s)));                      \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return Scalar(_##func(n, double(s)));                   \
    }
#else
    // With 'f' (float) appended to the name
    #define besselFunc(func)                                    \
    inline Scalar func(const Scalar s)                          \
    {                                                           \
        return ::func##f(s);                                    \
    }
    #define besselFunc2(func)                                   \
    inline Scalar func(const int n, const Scalar s)             \
    {                                                           \
        return ::func##f(n, s);                                 \
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
