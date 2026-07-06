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
    Foam::CompressibleThermalTurbulenceModel

Description
    Templated abstract base class for single-phase compressible
    thermal turbulence models.

SourceFiles
    CompressibleThermalTurbulenceModel.C

\*---------------------------------------------------------------------------*/

#ifndef CompressibleThermalTurbulenceModel_H
#define CompressibleThermalTurbulenceModel_H

#include "ThermalTurbulenceModel.H"
#include "compressibleThermalTurbulenceModel.H"
#include "compressibleTurbulenceModel.H"
#include "fvMatrix.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
               Class CompressibleThermalTurbulenceModel Declaration
\*---------------------------------------------------------------------------*/

template<class ThermoPhysicalModel, class TransportModel>
class CompressibleThermalTurbulenceModel
:
    public ThermalTurbulenceModel
    <
        geometricOneField,
        volScalarField,
        compressibleThermalTurbulenceModel,
        ThermoPhysicalModel,
	TransportModel
    >
{

public:

    typedef geometricOneField alphaField;
    typedef volScalarField rhoField;
    typedef ThermoPhysicalModel thermoPhysicalModel;
    typedef TransportModel transportModel;

    // Constructors

        //- Construct
        CompressibleThermalTurbulenceModel
        (
            const word& type,
            const geometricOneField& alpha,
            const volScalarField& rho,
            const volVectorField& U,
            const surfaceScalarField& alphaRhoPhi,
            const surfaceScalarField& phi,
            const ThermoPhysicalModel& thermo,
	    const TransportModel& transport,
            const word& propertiesName
        );


    // Selectors

        //- Return a reference to the selected turbulence model
        static autoPtr<CompressibleThermalTurbulenceModel> New
        (
	    const volScalarField& rho,
            const volVectorField& U,
            const surfaceScalarField& phi,
            const ThermoPhysicalModel& thermoPhysicalModel,
	          TransportModel& transportModel,
            const word& propertiesName = thermalTurbulenceModel::propertiesName
        );


    //- Destructor
    virtual ~CompressibleThermalTurbulenceModel() = default;


    // Member Functions

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "CompressibleThermalTurbulenceModel.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
