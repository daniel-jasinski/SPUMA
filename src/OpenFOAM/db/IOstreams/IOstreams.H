/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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

InClass
    Foam::IOstream

Description
    Useful combination of include files which define Sin, Sout and Serr and
    the use of IO streams generally.

\*---------------------------------------------------------------------------*/

#ifndef IOstreams_H
#define IOstreams_H

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#include "ISstream.H"
#include "prefixOSstream.H"
#include "token.H"
#include "char.H"
#include "int.H"
#include "uint.H"
#include "stdFoam.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //
// Global predefined streams for standard input, output

namespace Foam
{
    //- ISstream wrapped stdin (std::cin)
    FOAM_EXPORT_DATA extern ISstream Sin;

    //- OSstream wrapped stdout (std::cout)
    FOAM_EXPORT_DATA extern OSstream Sout;

    //- OSstream wrapped stderr (std::cerr)
    FOAM_EXPORT_DATA extern OSstream Serr;

    //- OSstream wrapped stdout (std::cout) with parallel prefix
    FOAM_EXPORT_DATA extern prefixOSstream Pout;

    //- OSstream wrapped stderr (std::cerr) with parallel prefix
    FOAM_EXPORT_DATA extern prefixOSstream Perr;
}

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
