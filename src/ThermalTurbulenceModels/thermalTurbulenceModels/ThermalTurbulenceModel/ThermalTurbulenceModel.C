/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2017 OpenFOAM Foundation
    Copyright (C) 2019-2021 OpenCFD Ltd.
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

#include "ThermalTurbulenceModel.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template
<
    class Alpha,
    class Rho,
    class BasicTurbulenceModel,
    class ThermoPhysicalModel,
    class TransportModel
>
Foam::ThermalTurbulenceModel<Alpha, Rho, BasicTurbulenceModel, ThermoPhysicalModel, TransportModel>::
ThermalTurbulenceModel
(
    const alphaField& alpha,
    const rhoField& rho,
    const volVectorField& U,
    const surfaceScalarField& alphaRhoPhi,
    const surfaceScalarField& phi,
    const thermoPhysicalModel& thermo,
    const transportModel& transport,
    const word& propertiesName
)
:
    BasicTurbulenceModel
    (
        rho,
        U,
        alphaRhoPhi,
        phi,
        propertiesName
    ),
    alpha_(alpha),
    thermo_(thermo),
    transport_(transport)
{}


// * * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * //

template
<
    class Alpha,
    class Rho,
    class BasicTurbulenceModel,
    class ThermoPhysicalModel,
    class TransportModel
>
Foam::autoPtr
<
    Foam::ThermalTurbulenceModel<Alpha, Rho, BasicTurbulenceModel, ThermoPhysicalModel, TransportModel>
>
Foam::ThermalTurbulenceModel<Alpha, Rho, BasicTurbulenceModel, ThermoPhysicalModel, TransportModel>::New
(
    const alphaField& alpha,
    const rhoField& rho,
    const volVectorField& U,
    const surfaceScalarField& alphaRhoPhi,
    const surfaceScalarField& phi,
    const thermoPhysicalModel& thermo,
    const transportModel& transport,
    const word& propertiesName
)
{
    const IOdictionary dict
    (
        IOobject
        (
            IOobject::groupName(propertiesName, alphaRhoPhi.group()),
            U.time().constant(),
            U.db(),
            IOobject::READ_IF_PRESENT,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        )
    );

    if (dict.size() > 0)
    {
        const word modelType(dict.get<word>("simulationType"));

        Info<< "Selecting thermal turbulence model type " << modelType << endl;

        auto* ctorPtr = dictionaryConstructorTable(modelType);

        if (!ctorPtr)
        {
            FatalIOErrorInLookup
            (
                dict,
                "simulationType",
                modelType,
                *dictionaryConstructorTablePtr_()
            ) << exit(FatalIOError);
        }

        return autoPtr<ThermalTurbulenceModel>
        (
            ctorPtr(alpha, rho, U, alphaRhoPhi, phi, thermo, transport, propertiesName)
        );
    }
    else
    {
        const word modelType = "RAS";

        Info<< "Selecting default thermal turbulence model type " << modelType << endl;

	auto* ctorPtr = dictionaryConstructorTable(modelType);

        return autoPtr<ThermalTurbulenceModel>
        (
            ctorPtr(alpha, rho, U, alphaRhoPhi, phi, thermo, transport, propertiesName)
        );
    }
}


// ************************************************************************* //
