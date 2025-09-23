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


namespace device
{

    inline label setNumBlocks(const label size, const label nThreads)
    {
        return (size + nThreads - 1) / nThreads;
    }

    inline label setTreeReduceNumBlocks(const label size, const label nThreads)
    {
        return (size + 2*nThreads- 1) / (2*nThreads);
    }
}

} // namespace Foam

#endif
