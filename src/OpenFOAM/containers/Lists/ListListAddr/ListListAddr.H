/*---------------------------------------------------------------------------*\
  *      .  *_______ * ______ .  __ *  __ * ___ .___    .  ___ .   *  .     *
    *  .    /       | |   _  \  |  |  |  | |   \/   | *   /   \ *   .    *   .
 *    .  * .\   (---*.|  |_)  |.|  |  |  |*|  \  /  |. * /  *  \  .  *     *
 =^^=^^==^^^=\   \^=^=|   ___/=^|  |^=|  |=|  |\/|  |^^=/  /=\  \^=^=^^===^^^=
 0  o  O  o---)   \ 0 |  |   0  |  o--o  |o|  |  |  | o/  _____  \ 0   o  O
     0    |_______/   |__| o   o \______/  |__| 0|__| /__/  o  \__\   o
  O   o  o        0  o      0   O        o    o       O  o     0   o    0  o
-------------------------------------------------------------------------------
    Copyright (C) 2026 Cineca
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

Class
    Foam::ListListAddr

Description
    Lightweight wrapper to access a list of lists on device.
    It uses the ListListAddr class to store the sizes and begin iterators of the inner lists.

SourceFiles
    ListListAddr.H
    ListListAddr.C

\*---------------------------------------------------------------------------*/

#include "List.H"
#include "tmp.H"
#include "autoPtr.H"


#ifndef Foam_ListListAddr_H
#define Foam_ListListAddr_H

namespace Foam
{

template<typename T> class ListListAddr;


template<typename T>
class ListListAddr
{
private:
    
    using iterator_type = const typename T::value_type*;

    bool usepool_;

    //- List of sizes of the inner lists
    autoPtr<labelList> sizes_;

    //- List of begin iterators of the inner lists
    autoPtr<List<iterator_type>> begins_;

public:

    //- Constructors
        constexpr ListListAddr() noexcept
        :
            sizes_(),
            begins_(),
            usepool_(false)
        {};
        
        ListListAddr(const UList<T>& listlist);
        
        ListListAddr(const ListListAddr& other);

        ListListAddr(ListListAddr<T>&& other);
    
    //- Destructor
        ~ListListAddr();

    // Member Functions
        bool usePool() const { return usepool_; }
        
        const labelList& sizes() const;
        
        List<iterator_type>& begins();

        const List<iterator_type>& begins() const;

    //- Clone
        autoPtr<ListListAddr> clone()
        {
            return autoPtr<ListListAddr>::New(*this);
        };


    //- Operators

        // move operator
        ListListAddr<T>& operator = (ListListAddr<T>&& other);

};

} // end namespace Foam

#ifdef NoRepository
    #include "ListListAddr.C"
#endif


#endif

// ************************************************************************* //

