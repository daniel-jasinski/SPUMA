/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2015-2016 OpenFOAM Foundation
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

Typedef
    Foam::compressible::thermalTurbulenceModel

Typedef
    Foam::compressible::RASModel

Typedef
    Foam::compressible::LESModel

Description
    Typedefs for turbulence, RAS and LES models for compressible flow
    based on the standard laminar transport package.

SourceFiles
    thermalTurbulentFluidThermoModel.C
    thermalTurbulentFluidThermoModels.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_thermalTurbulentFluidThermoModel_H
#define Foam_thermalTurbulentFluidThermoModel_H

#include "CompressibleThermalTurbulenceModel.H"
#include "compressibleTurbulenceModel.H"
#include "laminarThermalModel.H"
#include "fluidThermo.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    namespace compressible
    {
        typedef CompressibleThermalTurbulenceModel<fluidThermo, compressibleTurbulenceModel> thermalTurbulenceModel;
    }
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "thermalTurbulentFluidThermoModel.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
