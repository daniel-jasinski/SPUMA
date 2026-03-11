/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
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

#include "constantPrt.H"
#include "fvOptions.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace RASModels
{

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

template<class BasicTurbulenceModel>
tmp<volScalarField> constant<BasicTurbulenceModel>::k
(
    const tmp<volTensorField>& gradU
) const
{
    return tmp<volScalarField>
    (
        new volScalarField
        (
            IOobject
            (
                IOobject::groupName("k", this->alphaRhoPhi_.group()),
                this->runTime_.timeName(),
                this->mesh_
            ),
            this->mesh_
        )
    );
}


template<class BasicTurbulenceModel>
void constant<BasicTurbulenceModel>::correctAlphat()
{
    tmp<volScalarField> tnut = this->transport_.nut();
    const volScalarField& nut = tnut.cref();

    tmp<volScalarField> trho = this->thermo_.rho();
    const volScalarField& rho = trho.cref();

    const scalar Prt = this->Prt_.value();

    this->alphat_ = (rho * nut) / Prt;
    
    this->alphat_.correctBoundaryConditions();
    fv::options::New(this->mesh_).correct(this->alphat_);
    BasicTurbulenceModel::correctAlphat();
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
constant<BasicTurbulenceModel>::constant
(
    const alphaField& alpha,
    const rhoField& rho,
    const volVectorField& U,
    const surfaceScalarField& alphaRhoPhi,
    const surfaceScalarField& phi,
    const thermoPhysicalModel& thermo,
    const transportModel& transport,
    const word& propertiesName,
    const word& type
)
:
    thermalEddyViscosity<thermalRASModel<BasicTurbulenceModel>>
    (
        type,
        alpha,
        rho,
        U,
        alphaRhoPhi,
        phi,
	thermo,
        transport,
        propertiesName
    ),

    Prt_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "Prt",
            this->coeffDict_,
            0.85
        )
    )
{
    if (type == typeName)
    {
        this->printCoeffs(type);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
bool constant<BasicTurbulenceModel>::read()
{
    if (thermalEddyViscosity<thermalRASModel<BasicTurbulenceModel>>::read())
    {
        Prt_.readIfPresent(this->coeffDict());

        return true;
    }

    return false;
}


template<class BasicTurbulenceModel>
void constant<BasicTurbulenceModel>::correct()
{
    if (!this->turbulence_)
    {
        return;
    }

    thermalEddyViscosity<thermalRASModel<BasicTurbulenceModel>>::correct();
    correctAlphat();
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace RASModels
} // End namespace Foam

// ************************************************************************* //
