/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2016-2023 OpenCFD Ltd.
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
    Foam::Scalar

Description
    Floating-point number (float or double)

Note
    The floor/ceil/round operations could easily be extended to handle
    VectorSpace when the need arises. For example,

    \code
    template<class T>
    struct floorOp
    {
        T operator()(const T& x) const
        {
            T ret;
            for (direction cmpt=0; cmpt < pTraits<T>::nComponents; ++cmpt)
            {
                setComponent(ret, cmpt) = std::floor(component(x, cmpt));
            }
            return ret;
        }
    };
    \endcode

SourceFiles
    scalarImpl.txx

\*---------------------------------------------------------------------------*/

// Could also check if 'Scalar' is defined...
#ifndef Foam_use_scalarImpl_header
#error  "scalarImpl.H" is only to be included internally
#endif


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
#include "deviceM.H"

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Template specialisation for pTraits<Scalar>
template<>
class pTraits<Scalar>
{
    Scalar p_;

public:

    // Typedefs

        //- Component type
        typedef Scalar cmptType;

        //- Magnitude type
        typedef Scalar magType;

        //- Equivalent type of labels used for valid component indexing
        typedef label labelType;


    // Member Constants

        //- Dimensionality of space
        static constexpr direction dim = 3;

        //- Rank of Scalar is 0
        static constexpr direction rank = 0;

        //- Number of components in Scalar is 1
        static constexpr direction nComponents = 1;


    // Limits

        static constexpr Scalar min_ = -ScalarVGREAT;
        static constexpr Scalar max_ = ScalarVGREAT;
        static constexpr Scalar vsmall_ = ScalarVSMALL;


    // Static Data Members

        FOAM_EXPORT_DATA static const char* const typeName;
        static const char* typeName_() { return "scalar"; }
        FOAM_EXPORT_DATA static const char* const componentNames[];
        FOAM_EXPORT_DATA static const Scalar zero;
        FOAM_EXPORT_DATA static const Scalar one;
        FOAM_EXPORT_DATA static const Scalar max;
        FOAM_EXPORT_DATA static const Scalar min;
        FOAM_EXPORT_DATA static const Scalar rootMax;
        FOAM_EXPORT_DATA static const Scalar rootMin;
        FOAM_EXPORT_DATA static const Scalar vsmall;


    // Constructors

        //- Copy construct from primitive
        explicit pTraits(Scalar val) noexcept : p_(val) {}

        //- Read construct from Istream (uses tokenizer)
        explicit pTraits(Istream& is);


    // Member Functions

        //- Return the value
        operator Scalar() const noexcept { return p_; }

        //- Access the value
        operator Scalar&() noexcept { return p_; }
};


// * * * * * * * * * * * * * * * IO/Conversion * * * * * * * * * * * * * * * //

//- A word representation of a floating-point value.
//  Uses stringstream instead of std::to_string for more consistent formatting.
word name(const Scalar val);


//- A word representation of a floating-point value.
template<>
struct nameOp<Scalar>
{
    word operator()(const Scalar val) const
    {
        return Foam::name(val);
    }
};


//- Parse entire buffer as a float/double, skipping leading/trailing whitespace.
//  \return Parsed value or FatalIOError on any problem
Scalar ScalarRead(const char* buf);

//- Parse entire buffer as a float/double, skipping leading/trailing whitespace.
//  \return True if successful.
bool ScalarRead(const char* buf, Scalar& val);

//- Parse entire string as a float/double, skipping leading/trailing whitespace.
//  \return Parsed value or FatalIOError on any problem
inline Scalar ScalarRead(const std::string& str)
{
    return ScalarRead(str.c_str());
}

//- Parse entire string as a float/double, skipping leading/trailing whitespace.
//  \return True if successful.
inline bool ScalarRead(const std::string& str, Scalar& val)
{
    return ScalarRead(str.c_str(), val);
}


//- Read from Istream (uses tokenizer)
Scalar ScalarRead(Istream& is);
Istream& operator>>(Istream& is, Scalar& val);
Ostream& operator<<(Ostream& os, const Scalar val);


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

// Standard C++ transcendental functions
FOAM_DEVICE transFunc(sqrt)

FOAM_DEVICE transFunc(cbrt)
FOAM_DEVICE transFunc(exp)
FOAM_DEVICE transFunc(log)
FOAM_DEVICE transFunc(log10)
FOAM_DEVICE transFunc(sin)
FOAM_DEVICE transFunc(cos)
FOAM_DEVICE transFunc(tan)
FOAM_DEVICE transFunc(asin)
FOAM_DEVICE transFunc(acos)
FOAM_DEVICE transFunc(atan)
FOAM_DEVICE transFunc(sinh)
FOAM_DEVICE transFunc(cosh)
FOAM_DEVICE transFunc(tanh)
FOAM_DEVICE transFunc(asinh)
FOAM_DEVICE transFunc(acosh)
FOAM_DEVICE transFunc(atanh)

// Standard ANSI-C (but not in <cmath>) transcendental functions

FOAM_DEVICE transFunc(erf)
FOAM_DEVICE transFunc(erfc)
FOAM_DEVICE transFunc(lgamma)
FOAM_DEVICE transFunc(tgamma)

FOAM_DEVICE besselFunc(j0)
FOAM_DEVICE besselFunc(j1)
FOAM_DEVICE besselFunc(y0)
FOAM_DEVICE besselFunc(y1)
FOAM_DEVICE besselFunc2(jn)
FOAM_DEVICE besselFunc2(yn)


//- Non-const access to scalar-type (has no components)
FOAM_DEVICE inline Scalar& setComponent(Scalar& val, const direction) noexcept
{
    return val;
}


//- Return scalar value (has no components)
inline constexpr Scalar component(const Scalar val, const direction) noexcept
{
    return val;
}


//- Return 1 if s is greater_equal zero, or otherwise -1
FOAM_DEVICE inline Scalar sign(const Scalar s) noexcept
{
    return (s >= 0)? 1: -1;
}


//- Return 1 if s is greater than zero, otherwise 0
FOAM_DEVICE inline Scalar pos(const Scalar s) noexcept
{
    return (s > 0)? 1: 0;
}


//- Return 1 if s is greater_equal zero, or otherwise 0
FOAM_DEVICE inline Scalar pos0(const Scalar s) noexcept
{
    return (s >= 0)? 1: 0;
}


//- Return 1 if s is less than zero, or otherwise 0
FOAM_DEVICE inline Scalar neg(const Scalar s) noexcept
{
    return (s < 0)? 1: 0;
}


//- Return 1 if s is less_equal zero, or otherwise 0
FOAM_DEVICE inline Scalar neg0(const Scalar s) noexcept
{
    return (s <= 0)? 1: 0;
}


//- Return the positive part of s, otherwise zero. Same as max(0, s).
FOAM_DEVICE inline Scalar posPart(const Scalar s) noexcept
{
    return (s > 0)? s: 0;
}


//- Return the negative part of s, otherwise zero. Same as min(0, s).
//  Does not change the sign
FOAM_DEVICE inline Scalar negPart(const Scalar s) noexcept
{
    return (s < 0)? s: 0;
}


inline bool equal(const Scalar& a, const Scalar& b)
{
    return mag(a - b) <= ScalarVSMALL;
}


inline bool notEqual(const Scalar a, const Scalar b)
{
    return mag(a - b) > ScalarVSMALL;
}


//- Clamp scalar value to a 0-1 range
FOAM_DEVICE inline Scalar clamp(const Scalar val, Foam::zero_one)
{
    return (val < 0) ? 0 : (1 < val) ? 1 : val;
}


// Fast implementation, and with scalar promotion of upper/lower limits
FOAM_DEVICE inline Scalar clamp(const Scalar val, const Scalar lower, const Scalar upper)
{
    return (val < lower) ? lower : (upper < val) ? upper : val;
}


inline Scalar limit(const Scalar s1, const Scalar s2)
{
    return (mag(s1) < mag(s2)) ? s1: 0.0;
}


inline Scalar minMod(const Scalar s1, const Scalar s2)
{
    return (mag(s1) < mag(s2)) ? s1: s2;
}


//- Linear interpolation of scalars a and b by factor t
//  Explicit form (not fused multiply add etc) for exact values at end points.
//  Unlike std::lerp (C++20) there are no isfinite() checks etc.
FOAM_DEVICE inline constexpr Scalar lerp(const Scalar a, const Scalar b, const Scalar t)
{
    return (Scalar{1}-t)*a + t*b;
}


FOAM_DEVICE inline Scalar magSqr(const Scalar s)
{
    return s*s;
}


FOAM_DEVICE inline Scalar sqr(const Scalar s)
{
    return s*s;
}


FOAM_DEVICE inline Scalar pow3(const Scalar s)
{
    return s*sqr(s);
}


FOAM_DEVICE inline Scalar pow4(const Scalar s)
{
    return sqr(sqr(s));
}


FOAM_DEVICE inline Scalar pow5(const Scalar s)
{
    return s*pow4(s);
}


FOAM_DEVICE inline Scalar pow6(const Scalar s)
{
    return pow3(sqr(s));
}


FOAM_DEVICE inline Scalar pow025(const Scalar s)
{
    return sqrt(sqrt(s));
}


FOAM_DEVICE inline Scalar inv(const Scalar s)
{
    return 1.0/s;
}


FOAM_DEVICE inline Scalar dot(const Scalar s1, const Scalar s2)
{
    return s1*s2;
}


FOAM_DEVICE inline Scalar cmptMultiply(const Scalar s1, const Scalar s2)
{
    return s1*s2;
}


inline Scalar cmptPow(const Scalar s1, const Scalar s2)
{
    return pow(s1, s2);
}


FOAM_DEVICE inline Scalar cmptDivide(const Scalar s1, const Scalar s2)
{
    return s1/s2;
}


FOAM_DEVICE inline Scalar cmptMax(const Scalar s)
{
    return s;
}


FOAM_DEVICE inline Scalar cmptMin(const Scalar s)
{
    return s;
}


inline Scalar cmptAv(const Scalar s)
{
    return s;
}


inline Scalar cmptSqr(const Scalar s)
{
    return sqr(s);
}


FOAM_DEVICE inline Scalar cmptMag(const Scalar s)
{
    return mag(s);
}


inline Scalar cmptMagSqr(const Scalar s)
{
    return magSqr(s);
}


inline Scalar sqrtSumSqr(const Scalar a, const Scalar b)
{
    const Scalar maga = mag(a);
    const Scalar magb = mag(b);

    if (maga > magb)
    {
        return maga*sqrt(1.0 + sqr(magb/maga));
    }
    else
    {
        return magb < ScalarVSMALL ? 0.0 : magb*sqrt(1.0 + sqr(maga/magb));
    }
}


//- Stabilisation around zero for division
FOAM_DEVICE inline Scalar stabilise(const Scalar s, const Scalar tol)
{
    if (s >= 0)
    {
        return s + tol;
    }
    else
    {
        return s - tol;
    }
}


// Specializations

// Default definition in ops.H
template<class T> struct compareOp;

//- Compare scalar values
template<>
struct compareOp<Scalar>
{
    const Scalar tolerance;

    //- Construct with specified tolerance (non-negative value)
    compareOp(Scalar tol = ScalarVSMALL)
    :
        tolerance(tol)
    {}

    Scalar operator()(const Scalar& a, const Scalar& b) const
    {
        return (mag(a - b) <= tolerance) ? 0 : (a - b);
    }
};


// Default definition in ops.H
template<class T> struct equalOp;

//- Compare scalar values for equality
template<>
struct equalOp<Scalar>
{
    const Scalar tolerance;

    //- Construct with specified tolerance (non-negative value)
    equalOp(Scalar tol = ScalarVSMALL)
    :
        tolerance(tol)
    {}

    bool operator()(const Scalar& a, const Scalar& b) const
    {
        return mag(a - b) <= tolerance;
    }
};


// Default definition in ops.H
template<class T> struct notEqualOp;

//- Compare scalar values for inequality
template<>
struct notEqualOp<Scalar>
{
    const Scalar tolerance;

    //- Construct with specified tolerance (non-negative value)
    notEqualOp(Scalar tol = ScalarVSMALL)
    :
        tolerance(tol)
    {}

    bool operator()(const Scalar& a, const Scalar& b) const
    {
        return mag(a - b) > tolerance;
    }
};



// Default definition in ops.H (future?)
template<class T> struct floorOp;

//- Round scalar downwards - functor version of std::floor
template<>
struct floorOp<Scalar>
{
    Scalar operator()(const Scalar& x) const
    {
        return std::floor(x);
    }
};


// Default definition in ops.H (future?)
template<class T> struct ceilOp;

//- Round scalar upwards - functor version of std::ceil
template<>
struct ceilOp<Scalar>
{
    Scalar operator()(const Scalar& x) const
    {
        return std::ceil(x);
    }
};


// Default definition in ops.H (future?)
template<class T> struct roundOp;

//- Round scalar - functor version of std::round
template<>
struct roundOp<Scalar>
{
    Scalar operator()(const Scalar& x) const
    {
        return std::round(x);
    }
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
