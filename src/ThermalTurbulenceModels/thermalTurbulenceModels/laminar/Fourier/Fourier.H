/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2016 OpenFOAM Foundation
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

Class
    Foam::laminarModels::Fourier

Group
    grpLaminar

Description
    Thermal Turbulence model for laminar flow.

SourceFiles
    Fourier.C

\*---------------------------------------------------------------------------*/

#ifndef Fourier_H
#define Fourier_H

#include "laminarThermalModel.H"
#include "linearViscousThermalStress.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace laminarModels
{

/*---------------------------------------------------------------------------* \
                           Class Fourier Declaration
\*---------------------------------------------------------------------------*/

template<class BasicTurbulenceModel>
class Fourier
:
    public linearViscousThermalStress<laminarThermalModel<BasicTurbulenceModel>>
{

public:

    typedef typename BasicTurbulenceModel::alphaField alphaField;
    typedef typename BasicTurbulenceModel::rhoField rhoField;
    typedef typename BasicTurbulenceModel::thermoPhysicalModel thermoPhysicalModel; 
    typedef typename BasicTurbulenceModel::transportModel transportModel;


    //- Runtime type information
    TypeName("Fourier");


    // Constructors

        //- Construct from components
        Fourier
        (
            const alphaField& alpha,
            const rhoField& rho,
            const volVectorField& U,
            const surfaceScalarField& alphaRhoPhi,
            const surfaceScalarField& phi,
	    const thermoPhysicalModel& thermo,
            const transportModel& transport,
            const word& propertiesName = thermalTurbulenceModel::propertiesName
        );


    // Selectors

        //- Return a reference to the selected turbulence model
        static autoPtr<Fourier> New
        (
            const alphaField& alpha,
            const rhoField& rho,
            const volVectorField& U,
            const surfaceScalarField& alphaRhoPhi,
            const surfaceScalarField& phi,
	    const thermoPhysicalModel& thermo,
            const transportModel& transport,
            const word& propertiesName = thermalTurbulenceModel::propertiesName
        );


    //- Destructor
    virtual ~Fourier() = default;


    // Member Functions

        //- Const access to the coefficients dictionary
        virtual const dictionary& coeffDict() const;

        //- Read turbulenceProperties dictionary
        virtual bool read();

        //- Return the thermal turbulence viscosity, i.e. 0 for laminar flow
        virtual tmp<volScalarField> alphat() const;

        //- Return the thermal turbulence viscosity on patch
        virtual tmp<scalarField> alphat(const label patchi) const;

        //- Return the effective thermal diffusivity, i.e. the laminar thermal diffusivity
        virtual tmp<volScalarField> alphaEff() const;

        //- Return the effective thermal diffusivity on patch
        virtual tmp<scalarField> alphaEff(const label patchi) const;

        //- Correct the laminar viscosity
        virtual void correct();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace laminarModels
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "Fourier.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
