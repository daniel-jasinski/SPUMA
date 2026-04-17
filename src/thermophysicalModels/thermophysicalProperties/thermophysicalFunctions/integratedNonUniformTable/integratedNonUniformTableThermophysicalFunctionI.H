/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2020-2022 OpenFOAM Foundation
    Copyright (C) 2020 OpenCFD Ltd.
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


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

FOAM_DEVICE inline Foam::scalar Foam::thermophysicalFunctions::integratedNonUniformTable::intfdT
(
    scalar p,
    scalar T
) const
{
    const label i = index(p, T);
    const scalar Ti = values()[i].first();
    const scalar fi = values()[i].second();
    const scalar dT = T - Ti;
    const scalar lambda = dT/(values()[i + 1].first() - Ti);

    return
        intf_[i]
      + (fi + 0.5*lambda*(values()[i + 1].second() - fi))*dT;
}


// ************************************************************************* //
