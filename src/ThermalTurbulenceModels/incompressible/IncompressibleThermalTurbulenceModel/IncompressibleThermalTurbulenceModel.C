/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
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

#include "IncompressibleThermalTurbulenceModel.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class ThermoPhysicalModel, class TransportModel>
Foam::IncompressibleThermalTurbulenceModel<ThermoPhysicalModel, TransportModel>::
IncompressibleThermalTurbulenceModel
(
    const word& type,
    const geometricOneField& alpha,
    const geometricOneField& rho,
    const volVectorField& U,
    const surfaceScalarField& alphaRhoPhi,
    const surfaceScalarField& phi,
    const ThermoPhysicalModel& thermo,
    const TransportModel& transport,
    const word& propertiesName
)
:
    ThermalTurbulenceModel
    <
        geometricOneField,
        geometricOneField,
        incompressibleThermalTurbulenceModel,
        ThermoPhysicalModel,
	TransportModel
    >
    (
        alpha,
        rho,
        U,
        alphaRhoPhi,
        phi,
        thermo,
	transport,
        propertiesName
    )
{}


// * * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * //

template<class ThermoPhysicalModel, class TransportModel>
Foam::autoPtr<Foam::IncompressibleThermalTurbulenceModel<ThermoPhysicalModel, TransportModel>>
Foam::IncompressibleThermalTurbulenceModel<ThermoPhysicalModel, TransportModel>::New
(
    const volVectorField& U,
    const surfaceScalarField& phi,
    const ThermoPhysicalModel& thermo,
          transportModel& transport,
    const word& propertiesName
)
{
    return autoPtr<IncompressibleThermalTurbulenceModel>
    (
        static_cast<IncompressibleThermalTurbulenceModel*>(
        ThermalTurbulenceModel
        <
            geometricOneField,
            geometricOneField,
            incompressibleThermalTurbulenceModel,
            ThermoPhysicalModel,
	    TransportModel
        >::New
        (
            geometricOneField(),
            geometricOneField(),
            U,
            phi,
            phi,
            thermo,
	    transport,
            propertiesName
        ).ptr())
    );
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //


// ************************************************************************* //
