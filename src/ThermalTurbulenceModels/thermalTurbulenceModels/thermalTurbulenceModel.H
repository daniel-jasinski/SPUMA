/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2015 OpenFOAM Foundation
    Copyright (C) 2019-2023 OpenCFD Ltd.
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
    Foam::thermalTurbulenceModel

Group
    grpThermalTurbulence

Description
    Abstract base class for thermal turbulence models (RAS, LES and laminar).

SourceFiles
    thermalTurbulenceModel.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_thermalTurbulenceModel_H
#define Foam_thermalTurbulenceModel_H

#include "IOdictionary.H"
#include "primitiveFieldsFwd.H"
#include "volFieldsFwd.H"
#include "surfaceFieldsFwd.H"
#include "fvMatricesFwd.H"
#include "nearWallDist.H"
#include "geometricOneField.H"

// FOAM_THERMTURB_EXPORT_DATA: per-library Windows DLL data export macro for
// thermalTurbulenceModel statics. dllexport when compiling
// libthermalTurbulenceModels itself (FOAM_THERMTURB_BUILDING_LIB defined in
// its Make/options), dllimport in any consumer TU. Used instead of the
// generic FOAM_EXPORT_DATA from stdFoam.H because that macro's single
// FOAM_BUILDING_LIB flag would also re-export libOpenFOAM template
// instantiations from this library.
#if defined(_WIN32)
# if defined(FOAM_THERMTURB_BUILDING_LIB)
#  define FOAM_THERMTURB_EXPORT_DATA __declspec(dllexport)
# else
#  define FOAM_THERMTURB_EXPORT_DATA __declspec(dllimport)
# endif
#else
# define FOAM_THERMTURB_EXPORT_DATA
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declarations
class fvMesh;

/*---------------------------------------------------------------------------*\
                           Class thermalTurbulenceModel Declaration
\*---------------------------------------------------------------------------*/

class thermalTurbulenceModel
:
    public IOdictionary
{

protected:

    // Protected data

        const Time& runTime_;
        const fvMesh& mesh_;

        const volVectorField& U_;
        const surfaceScalarField& alphaRhoPhi_;
        const surfaceScalarField& phi_;

        //- Near wall distance boundary field
        nearWallDist y_;


private:

    // Private Member Functions

        //- No copy construct
        thermalTurbulenceModel(const thermalTurbulenceModel&) = delete;

        //- No copy assignment
        void operator=(const thermalTurbulenceModel&) = delete;


public:

    //- Runtime type information
    TypeName("thermalTurbulenceModel");

    //- Default name of the turbulence properties dictionary
    FOAM_THERMTURB_EXPORT_DATA static const word propertiesName;


    // Constructors

        //- Construct from components
        thermalTurbulenceModel
        (
            const volVectorField& U,
            const surfaceScalarField& alphaRhoPhi,
            const surfaceScalarField& phi,
            const word& propertiesName
        );


    //- Destructor
    virtual ~thermalTurbulenceModel() = default;


    // Member Functions

        //- Read model coefficients if they have changed
        virtual bool read() = 0;

        const Time& time() const
        {
            return runTime_;
        }

        const fvMesh& mesh() const
        {
            return mesh_;
        }

        //- Const access to the coefficients dictionary
        virtual const dictionary& coeffDict() const = 0;

        //- Helper function to return the name of the turbulence G field
        inline word GName() const
        {
            return IOobject::scopedName(type(), "G");
        }

        //- Access function to velocity field
        inline const volVectorField& U() const
        {
            return U_;
        }

        //- Access function to phase flux field
        inline const surfaceScalarField& alphaRhoPhi() const
        {
            return alphaRhoPhi_;
        }

        //- Return the volumetric flux field
        virtual tmp<surfaceScalarField> phi() const;

        //- Return the near wall distances
        const nearWallDist& y() const
        {
            return y_;
        }

        //- Return the laminar viscosity
        // virtual tmp<volScalarField> nu() const = 0;

        //- Return the laminar viscosity on patch
        // virtual tmp<scalarField> nu(const label patchi) const = 0;

        //- Return the thermal turbulence diffusivity
        virtual tmp<volScalarField> alphat() const = 0;

        //- Return the thermal turbulence diffusivity on patch
        virtual tmp<scalarField> alphat(const label patchi) const = 0;

        //- Return the effective thermal diffusivity
        virtual tmp<volScalarField> alphaEff() const = 0;

        //- Return the effective thermal diffusivity on patch
        virtual tmp<scalarField> alphaEff(const label patchi) const = 0;

        //- Return the laminar dynamic viscosity
        /*virtual tmp<volScalarField> mu() const = 0;

        //- Return the laminar dynamic viscosity on patch
        virtual tmp<scalarField> mu(const label patchi) const = 0;

        //- Return the turbulence dynamic viscosity
        virtual tmp<volScalarField> mut() const = 0;

        //- Return the turbulence dynamic viscosity on patch
        virtual tmp<scalarField> mut(const label patchi) const = 0;

        //- Return the effective dynamic viscosity
        virtual tmp<volScalarField> muEff() const = 0;

        //- Return the effective dynamic viscosity on patch
        virtual tmp<scalarField> muEff(const label patchi) const = 0;*/

        //- Return the turbulence kinetic energy
        virtual tmp<volScalarField> k() const = 0;

        //- Return the turbulence kinetic energy dissipation rate
        virtual tmp<volScalarField> epsilon() const = 0;

        //- Return the specific dissipation rate
        virtual tmp<volScalarField> omega() const = 0;

        //- Return the Reynolds stress tensor
        virtual tmp<volSymmTensorField> R() const = 0;

        //- Validate the turbulence fields after construction
        //  Update derived fields as required
        virtual void validate();

        //- Solve the turbulence equations and correct the turbulence viscosity
        virtual void correct() = 0;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
