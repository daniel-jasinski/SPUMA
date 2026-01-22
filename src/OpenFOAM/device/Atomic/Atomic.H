/*---------------------------------------------------------------------------*\
  *      .  *_______ * ______ .  __ *  __ * ___ .___    .  ___ .   *  .     *
    *  .    /       | |   _  \  |  |  |  | |   \/   | *   /   \ *   .    *   .
 *    .  * .\   (---*.|  |_)  |.|  |  |  |*|  \  /  |. * /  *  \  .  *     *
 =^^=^^==^^^=\   \^=^=|   ___/=^|  |^=|  |=|  |\/|  |^^=/  /=\  \^=^=^^===^^^=
 0  o  O  o---)   \ 0 |  |   0  |  o--o  |o|  |  |  | o/  _____  \ 0   o  O
     0    |_______/   |__| o   o \______/  |__| 0|__| /__/  o  \__\   o
  O   o  o        0  o      0   O        o    o       O  o     0   o    0  o
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of SPUMA.

    SPUMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SPUMA is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with SPUMA.  If not, see <http://www.gnu.org/licenses/>.

Struct
    Foam::Atomic

Description
    Front-end class to GPU atomic APIs

SourceFiles
    Atomic.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_Atomic_H
#define Foam_Atomic_H

#include "scalar.H"
#include "label.H"
#include "VectorSpace.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Struct Atomic Declaration
\*---------------------------------------------------------------------------*/


template <typename T>
struct Atomic
{
    template<typename Type1, typename Type2>
    FOAM_DEVICE static void AtomicAdd(Type1& x, const Type2& y)
    {
        T::_backendAtomicAdd(x,y);
    }

    template<class Form, class Cmpt, direction Ncmpts>
    FOAM_DEVICE static void AtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        T::_backendAtomicAdd(vs1,vs2);
    }

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
    }

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
    }

    FOAM_DEVICE static label AtomicCAS
    (
        label& x,
        const label& compare,
        const label& y
    )
    {
        return T::_backendAtomicCAS(x,compare,y);
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
