/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Description
    type utils to handle lambda arguments.

SourceFiles
    deviceUtils.H

\*---------------------------------------------------------------------------*/

#ifndef Foam_device_utils_H
#define Foam_device_utils_H

//#include "UList.H"
//#include "pTraits.H"
//#include "UniformList.H"

namespace Foam
{

//Fwd declarations
template<class T> class UList;
template<class T> class UniformList;
class one;

namespace argWrapper
{
    inline const one& cget(const one& ref ) noexcept {
        return ref;
    }

    template<typename T>
    inline const UniformList<T>& cget(const UniformList<T>& ref) noexcept{
        return ref;
    }

    template<typename T>
    inline const T* cget(const UList<T>& ref ) noexcept {
        return ref.cbegin();
    }
}; //namespace argWrapper

} // namespace Foam

#endif
