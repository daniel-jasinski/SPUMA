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
    List of optimal values for V-cycle error bound. 
    Reference:
    \verbatim
        D’Ambra, P., Durastante, F., Filippone, S. et al.
        Optimal polynomial smoothers for parallel AMG. Numer Algor (2025).
        https://doi.org/10.1007/s11075-025-02117-6
    \endverbatim

\*---------------------------------------------------------------------------*/

#ifndef Foam_lambdaOptMinList_H
#define Foam_lambdaOptMinList_H

#include "scalarList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

namespace Chebyshev
{

namespace FirstKind
{
    //- list of optimal values for V-cycle error bound
    const scalarList lambdaOptMinList = 
    {
        0.3333333264963328,
        0.1805358974013031,
        0.1159278474090664,
        0.0820780023852334,
        0.0618496081203676,
        0.0486605907936455,
        0.0395132940691538,
        0.0328701015026835,
        0.0278702889449934,
        0.0239987636410016,
        0.0209304530155615,
        0.0184513099814704,
        0.0164152156249015,
        0.0147195630514484,
        0.0132900883942254,
        0.0120723317797092,
        0.0110250964662371,
        0.0101170167811085,
        0.0093237783639166,
        0.0086261852644572
    };

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // namespace FirstKind

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // namespace Chebyshev

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //

