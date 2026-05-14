/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
    Copyright (C) 2026 Cineca
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

#include "faMeshLduAddressing.H"
#include "randomizedGraphColoring.H"


// * * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * //

const Foam::List<Foam::DynamicList<Foam::label>>&
Foam::faMeshLduAddressing::partitions(const dictionary& dict) const noexcept
{
    if (!partitions_)
    {
        partitions_ = std::make_unique<List<DynamicList<label>>>();

        autoPtr<graphColoring> alg = graphColoring::New
        (
            dict,
            *this,
            *partitions_
        );
        alg->execute();
    }

    return *partitions_;
}


// ************************************************************************* //

