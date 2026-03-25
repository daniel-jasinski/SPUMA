/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2014 OpenFOAM Foundation
    Copyright (C) 2019-2023 OpenCFD Ltd.
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

\*---------------------------------------------------------------------------*/

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

FOAM_DEVICE inline constexpr Foam::complex::complex() noexcept
:
    re(0),
    im(0)
{}


FOAM_DEVICE inline constexpr Foam::complex::complex(const Foam::zero) noexcept
:
    re(0),
    im(0)
{}


FOAM_DEVICE inline constexpr Foam::complex::complex(const scalar r) noexcept
:
    re(r),
    im(0)
{}


FOAM_DEVICE inline constexpr Foam::complex::complex(const scalar r, const scalar i) noexcept
:
    re(r),
    im(i)
{}


FOAM_DEVICE inline Foam::complex::complex(const std::complex<float>& c)
:
    re(c.real()),
    im(c.imag())
{}


FOAM_DEVICE inline Foam::complex::complex(const std::complex<double>& c)
:
    re(c.real()),
    im(c.imag())
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

inline Foam::complex Foam::complex::conjugate() const
{
    return complex(re, -im);
}


inline Foam::scalar Foam::complex::magnitude() const
{
    return std::hypot(re, im);
}


FOAM_DEVICE inline Foam::scalar Foam::complex::magSqr() const
{
    return (re*re + im*im);
}


inline Foam::scalar Foam::complex::cmptSum() const noexcept
{
    return (re + im);
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //

inline void Foam::complex::operator=(const Foam::zero)
{
    re = 0;
    im = 0;
}


inline void Foam::complex::operator=(const scalar s)
{
    re = s;
    im = 0;
}


FOAM_DEVICE inline void Foam::complex::operator+=(const complex& c)
{
    re += c.re;
    im += c.im;
}


FOAM_DEVICE inline void Foam::complex::operator+=(const scalar s)
{
    re += s;
}


inline void Foam::complex::operator-=(const complex& c)
{
    re -= c.re;
    im -= c.im;
}


inline void Foam::complex::operator-=(const scalar s)
{
    re -= s;
}


inline void Foam::complex::operator*=(const complex& c)
{
    *this = (*this)*c;
}


inline void Foam::complex::operator*=(const scalar s)
{
    re *= s;
    im *= s;
}


inline void Foam::complex::operator/=(const complex& c)
{
    *this = *this/c;
}


inline void Foam::complex::operator/=(const scalar s)
{
    re /= s;
    im /= s;
}


inline bool Foam::complex::operator==(const complex& c) const
{
    return (equal(re, c.re) && equal(im, c.im));
}


inline bool Foam::complex::operator!=(const complex& c) const
{
    return !operator==(c);
}


// * * * * * * * * * * * * * * * Global Operators  * * * * * * * * * * * * * //

inline Foam::complex Foam::operator~(const complex& c)
{
    return c.conjugate();
}


// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

namespace Foam
{

inline scalar mag(const complex& c)
{
    return c.magnitude();
}


inline scalar magSqr(const complex& c)
{
    return c.magSqr();
}


FOAM_DEVICE inline complex sqr(const complex& c)
{
    return c*c;
}


//- sgn() https://en.wikipedia.org/wiki/Sign_function#Complex_signum
inline complex sign(const complex& c)
{
    // TBD: Use volatile to avoid aggressive branch optimization
    const scalar s(mag(c));
    return s < ROOTVSMALL ? Zero : c/s;
}


//- csgn() https://en.wikipedia.org/wiki/Sign_function#Complex_signum
inline scalar csign(const complex& c)
{
    return equal(c.real(), 0) ? sign(c.imag()) : sign(c.real());
}


inline const complex& min(const complex& c1, const complex& c2)
{
    return (c1.magSqr() < c2.magSqr()) ? c1 : c2;
}


inline const complex& max(const complex& c1, const complex& c2)
{
    return (c1.magSqr() < c2.magSqr()) ? c2 : c1;
}


inline complex limit(const complex& c1, const complex& c2)
{
    return complex
    (
        limit(c1.real(), c2.real()),
        limit(c1.imag(), c2.imag())
    );
}


//- Linear interpolation of complex a and b by factor t
inline complex lerp(const complex& a, const complex& b, const scalar t)
{
    const scalar onet = (1-t);

    return complex
    (
        onet*a.real() + t*b.real(),
        onet*a.imag() + t*b.imag()
    );
}


inline const complex& sum(const complex& c)
{
    return c;
}


template<class Cmpt> class Tensor;

//- No-op rotational transform for complex
inline complex transform(const Tensor<scalar>&, const complex c)
{
    return c;
}


// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //

FOAM_DEVICE inline complex operator-(const complex& c)
{
    return complex(-c.re, -c.im);
}


FOAM_DEVICE inline complex operator+(const complex& c1, const complex& c2)
{
    return complex
    (
        c1.re + c2.re,
        c1.im + c2.im
    );
}


FOAM_DEVICE inline complex operator+(const complex& c, const scalar s)
{
    return complex(c.re + s, c.im);
}


FOAM_DEVICE inline complex operator+(const scalar s, const complex& c)
{
    return complex(c.re + s, c.im);
}


FOAM_DEVICE inline complex operator-(const complex& c1, const complex& c2)
{
    return complex
    (
        c1.re - c2.re,
        c1.im - c2.im
    );
}


FOAM_DEVICE inline complex operator-(const complex& c, const scalar s)
{
    return complex(c.re - s, c.im);
}


FOAM_DEVICE inline complex operator-(const scalar s, const complex& c)
{
    return complex(s - c.re, -c.im);
}


FOAM_DEVICE inline complex operator*(const complex& c1, const complex& c2)
{
    return complex
    (
        c1.re*c2.re - c1.im*c2.im,
        c1.im*c2.re + c1.re*c2.im
    );
}


FOAM_DEVICE inline complex operator*(const complex& c, const scalar s)
{
    return complex(s*c.re, s*c.im);
}


FOAM_DEVICE inline complex operator*(const scalar s, const complex& c)
{
    return complex(s*c.re, s*c.im);
}


FOAM_DEVICE inline complex operator/(const complex& c1, const complex& c2)
{
    const scalar sqrC2 = c2.magSqr();

    return complex
    (
        (c1.re*c2.re + c1.im*c2.im)/sqrC2,
        (c1.im*c2.re - c1.re*c2.im)/sqrC2
    );
}


FOAM_DEVICE inline complex operator/(const complex& c, const scalar s)
{
    return complex(c.re/s, c.im/s);
}


FOAM_DEVICE inline complex operator/(const scalar s, const complex& c)
{
    return complex(s)/c;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Complex transcendental functions
namespace Foam
{

// CUDA device: use __nv_* scalar functions to implement complex math
// (std::complex functions call MSVC internal _Exp etc. which don't exist on device)
#if defined(__CUDA_ARCH__)

extern "C" __device__ double __nv_exp(double);
extern "C" __device__ double __nv_log(double);
extern "C" __device__ double __nv_log10(double);
extern "C" __device__ double __nv_sin(double);
extern "C" __device__ double __nv_cos(double);
extern "C" __device__ double __nv_sinh(double);
extern "C" __device__ double __nv_cosh(double);
extern "C" __device__ double __nv_sqrt(double);
extern "C" __device__ double __nv_atan2(double, double);
extern "C" __device__ double __nv_hypot(double, double);
extern "C" __device__ double __nv_fabs(double);
extern "C" __device__ double __nv_pow(double, double);
extern "C" __device__ double __nv_asin(double);
extern "C" __device__ double __nv_acos(double);
extern "C" __device__ double __nv_atan(double);
extern "C" __device__ double __nv_asinh(double);
extern "C" __device__ double __nv_acosh(double);
extern "C" __device__ double __nv_atanh(double);

FOAM_DEVICE inline complex sqrt(const complex& c)
{
    const scalar a = c.real();
    const scalar b = c.imag();
    const scalar r = __nv_hypot(a, b);
    return complex
    (
        __nv_sqrt(0.5*(r + a)),
        (b >= 0 ? 1 : -1) * __nv_sqrt(0.5*(r - a))
    );
}

FOAM_DEVICE inline complex exp(const complex& c)
{
    const scalar er = __nv_exp(c.real());
    return complex(er * __nv_cos(c.imag()), er * __nv_sin(c.imag()));
}

FOAM_DEVICE inline complex log(const complex& c)
{
    return complex
    (
        __nv_log(__nv_hypot(c.real(), c.imag())),
        __nv_atan2(c.imag(), c.real())
    );
}

FOAM_DEVICE inline complex log10(const complex& c)
{
    // log10(z) = log(z) / log(10)
    const scalar ln10 = 2.302585092994046;
    return complex
    (
        __nv_log(__nv_hypot(c.real(), c.imag())) / ln10,
        __nv_atan2(c.imag(), c.real()) / ln10
    );
}

FOAM_DEVICE inline complex sin(const complex& c)
{
    return complex
    (
        __nv_sin(c.real()) * __nv_cosh(c.imag()),
        __nv_cos(c.real()) * __nv_sinh(c.imag())
    );
}

FOAM_DEVICE inline complex cos(const complex& c)
{
    return complex
    (
        __nv_cos(c.real()) * __nv_cosh(c.imag()),
        -__nv_sin(c.real()) * __nv_sinh(c.imag())
    );
}

FOAM_DEVICE inline complex tan(const complex& c)
{
    // tan(z) = sin(z)/cos(z)
    const complex s = sin(c);
    const complex cs = cos(c);
    return s / cs;
}

FOAM_DEVICE inline complex asin(const complex& c)
{
    // asin(z) = -i * log(iz + sqrt(1 - z^2))
    const scalar a = c.real();
    const scalar b = c.imag();
    const complex iz(-b, a);
    const complex s = sqrt(complex(1 - a*a + b*b, -2*a*b));
    const complex l = log(iz + s);
    return complex(l.imag(), -l.real());
}

FOAM_DEVICE inline complex acos(const complex& c)
{
    // acos(z) = -i * log(z + sqrt(z^2 - 1))
    const scalar a = c.real();
    const scalar b = c.imag();
    const complex s = sqrt(complex(a*a - b*b - 1, 2*a*b));
    const complex l = log(c + s);
    return complex(l.imag(), -l.real());
}

FOAM_DEVICE inline complex atan(const complex& c)
{
    // atan(z) = i/2 * log((1-iz)/(1+iz))
    const scalar a = c.real();
    const scalar b = c.imag();
    const complex iz(-b, a);
    const complex num(1 - iz.real(), -iz.imag());
    const complex den(1 + iz.real(), iz.imag());
    const complex l = log(num / den);
    return complex(-0.5*l.imag(), 0.5*l.real());
}

FOAM_DEVICE inline complex sinh(const complex& c)
{
    return complex
    (
        __nv_sinh(c.real()) * __nv_cos(c.imag()),
        __nv_cosh(c.real()) * __nv_sin(c.imag())
    );
}

FOAM_DEVICE inline complex cosh(const complex& c)
{
    return complex
    (
        __nv_cosh(c.real()) * __nv_cos(c.imag()),
        __nv_sinh(c.real()) * __nv_sin(c.imag())
    );
}

FOAM_DEVICE inline complex tanh(const complex& c)
{
    const complex s = sinh(c);
    const complex cs = cosh(c);
    return s / cs;
}

FOAM_DEVICE inline complex asinh(const complex& c)
{
    // asinh(z) = log(z + sqrt(z^2 + 1))
    const scalar a = c.real();
    const scalar b = c.imag();
    const complex s = sqrt(complex(a*a - b*b + 1, 2*a*b));
    return log(c + s);
}

FOAM_DEVICE inline complex acosh(const complex& c)
{
    // acosh(z) = log(z + sqrt(z^2 - 1))
    const scalar a = c.real();
    const scalar b = c.imag();
    const complex s = sqrt(complex(a*a - b*b - 1, 2*a*b));
    return log(c + s);
}

FOAM_DEVICE inline complex atanh(const complex& c)
{
    // atanh(z) = 0.5 * log((1+z)/(1-z))
    const complex num(1 + c.real(), c.imag());
    const complex den(1 - c.real(), -c.imag());
    return 0.5 * log(num / den);
}

// pow for complex on CUDA device
FOAM_DEVICE inline complex pow(const complex& x, const complex& y)
{
    // z^w = exp(w * log(z))
    const complex lx = log(x);
    return exp(complex
    (
        y.real()*lx.real() - y.imag()*lx.imag(),
        y.imag()*lx.real() + y.real()*lx.imag()
    ));
}

#define powFuncs(type2)                                                       \
    FOAM_DEVICE inline complex pow(const complex& x, const type2 y)           \
    {                                                                         \
        return pow(x, complex(scalar(y), 0));                                 \
    }                                                                         \
                                                                              \
    FOAM_DEVICE inline complex pow(const type2 x, const complex& y)           \
    {                                                                         \
        return pow(complex(scalar(x), 0), y);                                 \
    }

#else /* Host compilation */

    #define transFunc(func)                                                   \
    FOAM_DEVICE inline complex func(const complex& c)                         \
    {                                                                         \
        return std:: func (static_cast<std::complex<scalar>>(c));             \
    }

transFunc(sqrt)
transFunc(exp)
transFunc(log)
transFunc(log10)
transFunc(sin)
transFunc(cos)
transFunc(tan)
transFunc(asin)
transFunc(acos)
transFunc(atan)
transFunc(sinh)
transFunc(cosh)
transFunc(tanh)
transFunc(asinh)
transFunc(acosh)
transFunc(atanh)

// Special treatment for pow()
FOAM_DEVICE inline complex pow(const complex& x, const complex& y)
{
    return std::pow
    (
        static_cast<std::complex<scalar>>(x),
        static_cast<std::complex<scalar>>(y)
    );
}

// Combinations of complex and integral/float
#define powFuncs(type2)                                                       \
    inline complex pow(const complex& x, const type2 y)                       \
    {                                                                         \
        return std::pow(static_cast<std::complex<scalar>>(x), y);             \
    }                                                                         \
                                                                              \
    inline complex pow(const type2 x, const complex& y)                       \
    {                                                                         \
        return std::pow                                                       \
        (                                                                     \
            static_cast<std::complex<scalar>>(x),                             \
            static_cast<std::complex<scalar>>(y)                              \
        );                                                                    \
    }

#endif /* __CUDA_ARCH__ */

powFuncs(int)
powFuncs(long)
#if defined(__APPLE__) && WM_LABEL_SIZE == 64
powFuncs(int64_t)
#endif
powFuncs(float)
powFuncs(double)


FOAM_DEVICE inline complex pow3(const complex& c)
{
    return c*sqr(c);
}


FOAM_DEVICE inline complex pow4(const complex& c)
{
    return sqr(sqr(c));
}


FOAM_DEVICE inline complex pow5(const complex& c)
{
    return c*pow4(c);
}


FOAM_DEVICE inline complex pow6(const complex& c)
{
    return pow3(sqr(c));
}


FOAM_DEVICE inline complex pow025(const complex& c)
{
    return sqrt(sqrt(c));
}

} // End namespace Foam

#undef transFunc
#undef powFuncs

// ************************************************************************* //
