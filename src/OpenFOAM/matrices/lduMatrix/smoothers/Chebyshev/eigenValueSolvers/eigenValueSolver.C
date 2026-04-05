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

\*---------------------------------------------------------------------------*/

#include "eigenValueSolver.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(eigenValueSolver, 0);
    defineRunTimeSelectionTable(eigenValueSolver, word);
}

// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * /

Foam::autoPtr<Foam::eigenValueSolver> Foam::eigenValueSolver::New
(
    Istream& stream
)
{
    const word type(stream);

    auto* ctorPtr = wordConstructorTable(type);

    if (!ctorPtr)
    {
        FatalErrorInLookup
        (
            "eigenValueSolver",
            type,
            *wordConstructorTablePtr_()
        ) << abort(FatalError);
    }

    return autoPtr<eigenValueSolver>
    (
        ctorPtr(stream)
    );
};

// ************************************************************************* //
