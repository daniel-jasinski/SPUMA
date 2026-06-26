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

\*---------------------------------------------------------------------------*/

#ifndef Foam_ListListAddr_C
#define Foam_ListListAddr_C


#include "ListListAddr.H"
#include "ListListOps.H"

// * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * * //
template<typename T>
Foam::ListListAddr<T>::ListListAddr(const Foam::UList<T>& listlist)
:
usepool_(ListListOps::subUsePool(listlist,accessOp<T>()))
{

    const label N = listlist.size();
    if (N <= 0)
    {
        FatalErrorInFunction
            << "Empty list of lists provided to ListListAddr constructor"
            << abort(FatalError);
        return;
    }
    
    this->sizes_ = autoPtr<labelList>::New(N,poolSwitch(usepool_));
    this->begins_ = autoPtr<List<iterator_type>>::New(N,poolSwitch(usepool_));

    auto& sizeRef = sizes_.ref();
    auto& beginRef = begins_.ref();

    if (usepool_)
    {   
        foamExecutor exec;
        auto sizePtr = sizeRef.begin();
        auto beginPtr = beginRef.begin();
        auto listlistPtr = listlist.cbegin();
        auto Lambda = [=](label i)
        {
            sizePtr[i] = listlistPtr[i].size();
            beginPtr[i] = listlistPtr[i].begin();
        };
        exec.parallelFor(Lambda,N);
    }
    else
    {
        for(label i = 0; i < N; i++)
        {
            sizeRef[i] = listlist[i].size();
            beginRef[i] = listlist[i].begin();
        }
    }

};


template<typename T>
Foam::ListListAddr<T>::ListListAddr(Foam::ListListAddr<T>&& other)
{
    this->sizes_ = std::move(other.sizes_);
    this->begins_ = std::move(other.begins_);
};


template<typename T>
Foam::ListListAddr<T>::ListListAddr(const Foam::ListListAddr<T>& other)
:
usepool_(other.usePool())
{
    this->sizes_ = autoPtr<labelList>::New(other.sizes());
    this->begins_ = autoPtr<List<iterator_type>>::New(other.begins());
};


// * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * * //
template<typename T>
Foam::ListListAddr<T>::~ListListAddr()
{
    sizes_.clear();
    begins_.clear();
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //
template<typename T>
const Foam::labelList& Foam::ListListAddr<T>::sizes() const
{
    return this->sizes_();
}


template<typename T>
Foam::List<typename Foam::ListListAddr<T>::iterator_type>& 
Foam::ListListAddr<T>::begins()
{
    return this->begins_();
}


template<typename T>
const Foam::List<typename Foam::ListListAddr<T>::iterator_type>& 
Foam::ListListAddr<T>::begins() const
{
    return this->begins_();
}


// * * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * //
template<typename T>
Foam::ListListAddr<T>& Foam::ListListAddr<T>::operator = (Foam::ListListAddr<T>&& other)
{
    if (this != &other)
    {
        this->sizes_ = std::move(other.sizes_);
        this->begins_ = std::move(other.begins_);
    }
    return *this;
}

#endif

// ************************************************************************* //

