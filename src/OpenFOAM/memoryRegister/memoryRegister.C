/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2024 AUTHOR,AFFILIATION
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

#include "memoryRegister.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

Foam::memoryRegister* Foam::memoryRegister::v_(nullptr);



// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::memoryRegister::memoryRegister()
:
    totalSize_(0)
{
    #ifdef have_cuda
    exec_ = new cudaRegisterExecutor();
    #endif
}



// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

Foam::memoryRegister*
Foam::memoryRegister::New()
{
    if (!v_)
    {
        v_ = new memoryRegister();
    }
    
    return v_;
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::memoryRegister::~memoryRegister()
{
    delete exec_;
}


// ************************************************************************* //
