/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016 OpenFOAM Foundation
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

Class
    Foam::laminarThermalModel

Description
    Templated abstract base class for laminar thermal transport models

SourceFiles
    laminarThermalModel.C

\*---------------------------------------------------------------------------*/

#ifndef laminarThermalModel_H
#define laminarThermalModel_H

#include "ThermalTurbulenceModel.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class laminarThermalModel Declaration
\*---------------------------------------------------------------------------*/

template<class BasicTurbulenceModel>
class laminarThermalModel
:
    public BasicTurbulenceModel
{

protected:

    // Protected data

        //- laminar coefficients dictionary
        dictionary laminarDict_;

        //- Flag to print the model coeffs at run-time
        Switch printCoeffs_;

        //- Model coefficients dictionary
        dictionary coeffDict_;


    // Protected Member Functions

        //- Print model coefficients
        virtual void printCoeffs(const word& type);

        //- No copy construct
        laminarThermalModel(const laminarThermalModel&) = delete;

        //- No copy assignment
        void operator=(const laminarThermalModel&) = delete;


public:

    typedef typename BasicTurbulenceModel::alphaField alphaField;
    typedef typename BasicTurbulenceModel::rhoField rhoField;
    typedef typename BasicTurbulenceModel::thermoPhysicalModel thermoPhysicalModel;
    typedef typename BasicTurbulenceModel::transportModel transportModel;

    //- Runtime type information
    TypeName("laminar");


    // Declare run-time constructor selection table

        declareRunTimeSelectionTable
        (
            autoPtr,
            laminarThermalModel,
            dictionary,
            (
                const alphaField& alpha,
                const rhoField& rho,
                const volVectorField& U,
                const surfaceScalarField& alphaRhoPhi,
                const surfaceScalarField& phi,
		const thermoPhysicalModel& thermo,
                const transportModel& transport,
                const word& propertiesName
            ),
            (alpha, rho, U, alphaRhoPhi, phi, thermo, transport, propertiesName)
        );


    // Constructors

        //- Construct from components
        laminarThermalModel
        (
            const word& type,
            const alphaField& alpha,
            const rhoField& rho,
            const volVectorField& U,
            const surfaceScalarField& alphaRhoPhi,
            const surfaceScalarField& phi,
	    const thermoPhysicalModel& thermo,
            const transportModel& transport,
            const word& propertiesName
        );


    // Selectors

        //- Return a reference to the selected laminar model
        static autoPtr<laminarThermalModel> New
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
    virtual ~laminarThermalModel() = default;


    // Member Functions

        //- Read model coefficients if they have changed
        virtual bool read();


        // Access

            //- Const access to the coefficients dictionary
            virtual const dictionary& coeffDict() const
            {
                return coeffDict_;
            }

            //- Return the turbulence viscosity, i.e. 0 for laminar flow
            virtual tmp<volScalarField> alphat() const;

            //- Return the turbulence viscosity on patch
            virtual tmp<scalarField> alphat(const label patchi) const;

            //- Return the effective thermal diffusivity, i.e. the laminar thermal diffusivity
            virtual tmp<volScalarField> alphaEff() const;

            //- Return the effective thermal diffusivity on patch
            virtual tmp<scalarField> alphaEff(const label patchi) const;

            //- Return the turbulence kinetic energy, i.e. 0 for laminar flow
            virtual tmp<volScalarField> k() const;

            //- Return the turbulence kinetic energy dissipation rate,
            //  i.e. 0 for laminar flow
            virtual tmp<volScalarField> epsilon() const;

            //- Return the specific dissipation rate, i.e. 0 for laminar flow
            virtual tmp<volScalarField> omega() const;

            //- Return the Reynolds stress tensor, i.e. 0 for laminar flow
            virtual tmp<volSymmTensorField> R() const;

            //- Correct the laminar transport
            virtual void correct();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "laminarThermalModel.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
