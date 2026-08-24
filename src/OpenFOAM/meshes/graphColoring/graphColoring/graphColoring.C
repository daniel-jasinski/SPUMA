/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

#include "graphColoring.H"


// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(graphColoring, 0);
    defineRunTimeSelectionTable(graphColoring, dictionary);
}


// * * * * * * * * * * * * * Static Member Functions * * * * * * * * * * * * //

Foam::autoPtr<Foam::graphColoring> Foam::graphColoring::New
(
    const dictionary& dict,
    const lduAddressing& addr,
    List<DynamicList<label>>& partitions
)
{
    // Look up the keyword "type" in the dictionary
    const word graphColoringName(dict.getOrDefault<word>("graphColoring", "randomized"));

    // Search the table for the keyword
    auto* ctorPtr = dictionaryConstructorTable(graphColoringName);

   if (!ctorPtr)
    {
        FatalIOErrorInLookup
        (
            dict,
            "graphColoring",
            graphColoringName,
            *dictionaryConstructorTablePtr_()
        ) << exit(FatalIOError);
    }

    // Instantiate and return the child
    return autoPtr<graphColoring>
    (
        ctorPtr
        (
            dict,
            addr,
            partitions
        )
    );
}


// ************************************************************************* //