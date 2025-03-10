/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 CINECA
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
    template<typename Type>
    FOAM_DEVICE static void AtomicAdd(Type& x,const Type& y)
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
