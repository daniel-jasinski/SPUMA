/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2015 OpenFOAM Foundation
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
    Foam::incompressibleThermalTurbulenceModel

Description
    Abstract base class for thermal turbulence models (RAS, laminar).

SourceFiles
    incompressibleThermalTurbulenceModel.C

\*---------------------------------------------------------------------------*/

#ifndef incompressibleThermalTurbulenceModel_H
#define incompressibleThermalTurbulenceModel_H

#include "thermalTurbulenceModel.H"
#include "geometricOneField.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward declarations
class fvMesh;

/*---------------------------------------------------------------------------*\
                Class incompressibleThermalTurbulenceModel Declaration
\*---------------------------------------------------------------------------*/

class incompressibleThermalTurbulenceModel
:
    public thermalTurbulenceModel
{

protected:

    // Protected data

        geometricOneField rho_;


    // Protected member functions

        //- ***HGW Temporary function to be removed when the run-time selectable
        //  thermal transport layer is complete
        virtual void correctAlphat()
        {}


private:

    // Private Member Functions

        //- No copy construct
        incompressibleThermalTurbulenceModel
        (
            const incompressibleThermalTurbulenceModel&
        ) = delete;

        //- No copy assignment
        void operator=(const incompressibleThermalTurbulenceModel&) = delete;


public:

    //- Runtime type information
    TypeName("incompressibleThermalTurbulenceModel");


    // Constructors

        //- Construct from components
        incompressibleThermalTurbulenceModel
        (
            const geometricOneField& rho,
            const volVectorField& U,
            const surfaceScalarField& alphaRhoPhi,
            const surfaceScalarField& phi,
            const word& propertiesName
        );


    //- Destructor
    virtual ~incompressibleThermalTurbulenceModel() = default;


    // Member Functions

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
