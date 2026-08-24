/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
    Copyright (C) 2022 OpenCFD Ltd.
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

#include "thermalTurbulentFluidThermoModels.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

defineThermalTurbulenceModelTypes
(
    geometricOneField,
    volScalarField,
    compressibleThermalTurbulenceModel,
    CompressibleThermalTurbulenceModel,
    fluidThermo,
    compressibleTurbulenceModel
);

makeBaseThermalTurbulenceModel
(
    geometricOneField,
    volScalarField,
    compressibleThermalTurbulenceModel,
    CompressibleThermalTurbulenceModel,
    fluidThermo,
    compressibleTurbulenceModel
);


// -------------------------------------------------------------------------- //
// Laminar models
// -------------------------------------------------------------------------- //

#include "Fourier.H"
makeLaminarThermalModel(Fourier);

// -------------------------------------------------------------------------- //
// RAS models
// -------------------------------------------------------------------------- //

#include "constantPrt.H"
makeRASThermalModel(constant);

#include "Kays.H"
makeRASThermalModel(Kays);

// -------------------------------------------------------------------------- //
// LES models
// -------------------------------------------------------------------------- //

// ************************************************************************* //
