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
    Foam::cpuAtomic

Description
    cpu Atomic backend

SourceFiles
    cpuAtomic.H

\*---------------------------------------------------------------------------*/


#ifndef Foam_cpu_Atomic_H
#define Foam_cpu_Atomic_H

#include "Atomic.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Struct cpuAtomic Declaration
\*---------------------------------------------------------------------------*/


struct cpuAtomic
: Atomic<cpuAtomic>
{
    template<typename T>
    static void  _backendAtomicAdd(T& x, const T& y)
    {
        x += y;
    }

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicAdd
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        vs1 += vs2;
    }

    template<typename T>
    static void  _backendAtomicMax(T& x, const T& y)
    {
        x = max(x,y);
    }

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMax
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        vs1 = max(vs1,vs2);
    }

    template<typename T>
    static void  _backendAtomicMin(T& x, const T& y)
    {
        x = min(x,y);
    }

    template<class Form, class Cmpt, direction Ncmpts>
    static void _backendAtomicMin
    (
        VectorSpace<Form, Cmpt, Ncmpts>& vs1,
        const VectorSpace<Form, Cmpt, Ncmpts>& vs2
    )
    {
        vs1 = min(vs1,vs2);
    }

    static label _backendAtomicCAS
    (
        label& x,
        const label& compare,
        const label& y
    )
    {
        label old = x;
        x = (x == compare ? y : x);
        return old;
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
