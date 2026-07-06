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

#include "Kays.H"
#include "fvOptions.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace RASModels
{

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

template<class BasicTurbulenceModel>
tmp<volScalarField> Kays<BasicTurbulenceModel>::k
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
void Kays<BasicTurbulenceModel>::correctAlphat()
{
    tmp<volScalarField> tnut = this->transport_.nut();
    const volScalarField& nut = tnut.cref();
    auto nutPtr = nut.cbegin();

    auto alphatPtr = this->alphat_.begin();

    tmp<volScalarField> tmu = this->transport_.mu();
    const volScalarField& mu = tmu.cref();
    auto muPtr = mu.cbegin();

    tmp<volScalarField> tnu = this->thermo_.nu();
    const volScalarField& nu = tnu.cref();
    auto nuPtr = nu.cbegin();

    tmp<volScalarField> trho = this->thermo_.rho();
    const volScalarField& rho = trho.cref();
    auto rhoPtr = rho.cbegin();

    tmp<volScalarField> tcp = this->thermo_.Cp();
    const volScalarField& cp = tcp.cref();
    auto cpPtr = cp.cbegin();

    tmp<volScalarField> tkappa = this->thermo_.kappa();
    const volScalarField& kappa = tkappa.cref();
    auto kappaPtr = kappa.cbegin();

    const scalar Prt = this->PrtInfty_.value();

    foamExecutor exec;
    auto Lambda = [=](label celli)
    {
        const scalar Pr = (cpPtr[celli] * muPtr[celli]) / kappaPtr[celli];
        const scalar IKaysPrt = (nutPtr[celli] * Pr) / (Prt * nutPtr[celli] * Pr + 0.7 * nuPtr[celli]);
        alphatPtr[celli] = rhoPtr[celli] * nutPtr[celli] * IKaysPrt;
    };
    exec.parallelFor(Lambda, this->alphat_.size());

    this->alphat_.correctBoundaryConditions();
    fv::options::New(this->mesh_).correct(this->alphat_);
    BasicTurbulenceModel::correctAlphat();
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
Kays<BasicTurbulenceModel>::Kays
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

    PrtInfty_
    (
        dimensioned<scalar>::getOrAddToDict
        (
            "PrtInfty",
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
bool Kays<BasicTurbulenceModel>::read()
{
    if (thermalEddyViscosity<thermalRASModel<BasicTurbulenceModel>>::read())
    {
        PrtInfty_.readIfPresent(this->coeffDict());

        return true;
    }

    return false;
}


template<class BasicTurbulenceModel>
void Kays<BasicTurbulenceModel>::correct()
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
