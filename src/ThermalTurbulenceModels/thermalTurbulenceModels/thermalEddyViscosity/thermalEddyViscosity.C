/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2013-2017 OpenFOAM Foundation
    Copyright (C) 2023 OpenCFD Ltd.
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

#include "thermalEddyViscosity.H"
#include "fvc.H"
#include "fvm.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
Foam::thermalEddyViscosity<BasicTurbulenceModel>::thermalEddyViscosity
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
)
:
    linearViscousThermalStress<BasicTurbulenceModel>
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

    alphat_
    (
        IOobject
        (
            IOobject::groupName("alphat", alphaRhoPhi.group()),
            this->runTime_.timeName(),
            this->mesh_,
            IOobject::MUST_READ,
            IOobject::AUTO_WRITE,
            IOobject::REGISTER
        ),
        this->mesh_
    )
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
bool Foam::thermalEddyViscosity<BasicTurbulenceModel>::read()
{
    return BasicTurbulenceModel::read();
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::volSymmTensorField>
Foam::thermalEddyViscosity<BasicTurbulenceModel>::R() const
{
    tmp<volScalarField> tk(k());

    // Get list of patchField type names from k
    wordList patchFieldTypes(tk().boundaryField().types());

    // For k patchField types which do not have an equivalent for symmTensor
    // set to calculated
    forAll(patchFieldTypes, i)
    {
        if
        (
           !fvPatchField<symmTensor>::patchConstructorTablePtr_()
                ->contains(patchFieldTypes[i])
        )
        {
            patchFieldTypes[i] = fvPatchFieldBase::calculatedType();
        }
    }

    return volSymmTensorField::New
    (
        IOobject::groupName("R", this->alphaRhoPhi_.group()),
        IOobject::NO_REGISTER,
        ((2.0/3.0)*I)*tk() - (alphat_)*devTwoSymm(fvc::grad(this->U_)),
        patchFieldTypes
    );
}


template<class BasicTurbulenceModel>
void Foam::thermalEddyViscosity<BasicTurbulenceModel>::validate()
{
    correctAlphat();
}


template<class BasicTurbulenceModel>
void Foam::thermalEddyViscosity<BasicTurbulenceModel>::correct()
{
    BasicTurbulenceModel::correct();
}


// ************************************************************************* //
