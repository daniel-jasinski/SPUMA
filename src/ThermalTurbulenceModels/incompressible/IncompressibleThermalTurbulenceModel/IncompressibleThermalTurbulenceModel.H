/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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

Class
    Foam::IncompressibleThermalTurbulenceModel

Description
    Templated abstract base class for single-phase incompressible
    thermal turbulence models.

SourceFiles
    IncompressibleThermalTurbulenceModel.C

\*---------------------------------------------------------------------------*/

#ifndef IncompressibleThermalTurbulenceModel_H
#define IncompressibleThermalTurbulenceModel_H

#include "ThermalTurbulenceModel.H"
#include "incompressibleThermalTurbulenceModel.H"
#include "IncompressibleTurbulenceModel.H"
#include "fvMatrix.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
               Class IncompressibleThermalTurbulenceModel Declaration
\*---------------------------------------------------------------------------*/

template<class ThermoPhysicalModel, class TransportModel>
class IncompressibleThermalTurbulenceModel
:
    public ThermalTurbulenceModel
    <
        geometricOneField,
        geometricOneField,
        incompressibleThermalTurbulenceModel,
        ThermoPhysicalModel,
	TransportModel
    >
{

public:

    typedef geometricOneField alphaField;
    typedef geometricOneField rhoField;
    typedef ThermoPhysicalModel thermoPhysicalModel;
    typedef TransportModel transportModel;

    // Constructors

        //- Construct
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
        );


    // Selectors

        //- Return a reference to the selected turbulence model
        static autoPtr<IncompressibleThermalTurbulenceModel> New
        (
            const volVectorField& U,
            const surfaceScalarField& phi,
            const ThermoPhysicalModel& thermoPhysicalModel,
	          TransportModel& transportModel,
            const word& propertiesName = thermalTurbulenceModel::propertiesName
        );


    //- Destructor
    virtual ~IncompressibleThermalTurbulenceModel() = default;


    // Member Functions

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "IncompressibleThermalTurbulenceModel.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
