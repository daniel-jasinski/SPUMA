/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 OpenFOAM Foundation
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

#include "laminarThermalModel.H"
#include "Fourier.H"

// * * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * //

template<class BasicTurbulenceModel>
void Foam::laminarThermalModel<BasicTurbulenceModel>::printCoeffs(const word& type)
{
    if (printCoeffs_)
    {
        Info<< coeffDict_.dictName() << coeffDict_ << endl;
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
Foam::laminarThermalModel<BasicTurbulenceModel>::laminarThermalModel
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
    BasicTurbulenceModel
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

    laminarDict_(this->subOrEmptyDict("laminar")),
    printCoeffs_(laminarDict_.getOrDefault<Switch>("printCoeffs", false)),
    coeffDict_(laminarDict_.optionalSubDict(type + "Coeffs"))
{
    // Force the construction of the mesh deltaCoeffs which may be needed
    // for the construction of the derived models and BCs
    this->mesh_.deltaCoeffs();
}


// * * * * * * * * * * * * * * * * Selectors * * * * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
Foam::autoPtr<Foam::laminarThermalModel<BasicTurbulenceModel>>
Foam::laminarThermalModel<BasicTurbulenceModel>::New
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
    const IOdictionary modelDict
    (
        IOobject
        (
            IOobject::groupName(propertiesName, alphaRhoPhi.group()),
            U.time().constant(),
            U.db(),
            IOobject::MUST_READ,
            IOobject::NO_WRITE,
            IOobject::NO_REGISTER
        )
    );

    const dictionary* dictptr = modelDict.findDict("laminar");

    if (dictptr)
    {
        const dictionary& dict = *dictptr;

        const word modelType
        (
            // laminarThermalModel -> model (after v2006)
            dict.getCompat<word>("model", {{"laminarThermalModel", -2006}})
        );

        Info<< "Selecting laminar thermal stress model " << modelType << endl;

        auto* ctorPtr = dictionaryConstructorTable(modelType);

        if (!ctorPtr)
        {
            FatalIOErrorInLookup
            (
                dict,
                "laminar thermal model",
                modelType,
                *dictionaryConstructorTablePtr_()
            ) << exit(FatalIOError);
        }

        return autoPtr<laminarThermalModel>
        (
            ctorPtr
            (
                alpha,
                rho,
                U,
                alphaRhoPhi,
                phi,
		thermo,
                transport, 
		propertiesName)
        );
    }
    else
    {
        Info<< "Selecting laminar thermal stress model "
            << laminarModels::Fourier<BasicTurbulenceModel>::typeName << endl;

        return autoPtr<laminarThermalModel>
        (
            new laminarModels::Fourier<BasicTurbulenceModel>
            (
                alpha,
                rho,
                U,
                alphaRhoPhi,
                phi,
                thermo,
                transport,
                propertiesName
            )
        );
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class BasicTurbulenceModel>
bool Foam::laminarThermalModel<BasicTurbulenceModel>::read()
{
    if (BasicTurbulenceModel::read())
    {
        laminarDict_ <<= this->subDict("laminar");

        coeffDict_ <<= laminarDict_.optionalSubDict(type() + "Coeffs");

        return true;
    }

    return false;
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::volScalarField>
Foam::laminarThermalModel<BasicTurbulenceModel>::alphat() const
{
    return volScalarField::New
    (
        IOobject::groupName("alphat", this->alphaRhoPhi_.group()),
        IOobject::NO_REGISTER,
        this->mesh_,
        dimensionedScalar(dimViscosity, Zero)
    );
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::scalarField>
Foam::laminarThermalModel<BasicTurbulenceModel>::alphat
(
    const label patchi
) const
{
    return tmp<scalarField>::New(this->mesh_.boundary()[patchi].size(), Zero);
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::volScalarField>
Foam::laminarThermalModel<BasicTurbulenceModel>::alphaEff() const
{
    return volScalarField::New
    (
        IOobject::groupName("alphaEff", this->alphaRhoPhi_.group()),
        IOobject::NO_REGISTER,
        this->thermo().alpha()
    );
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::scalarField>
Foam::laminarThermalModel<BasicTurbulenceModel>::alphaEff
(
    const label patchi
) const
{
    return this->thermo().alpha(patchi);
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::volScalarField>
Foam::laminarThermalModel<BasicTurbulenceModel>::k() const
{
    return volScalarField::New
    (
        IOobject::groupName("k", this->alphaRhoPhi_.group()),
        IOobject::NO_REGISTER,
        this->mesh_,
        dimensionedScalar(sqr(this->U_.dimensions()), Zero)
    );
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::volScalarField>
Foam::laminarThermalModel<BasicTurbulenceModel>::epsilon() const
{
    return volScalarField::New
    (
        IOobject::groupName("epsilon", this->alphaRhoPhi_.group()),
        IOobject::NO_REGISTER,
        this->mesh_,
        dimensionedScalar(sqr(this->U_.dimensions())/dimTime, Zero)
    );
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::volScalarField>
Foam::laminarThermalModel<BasicTurbulenceModel>::omega() const
{
    return volScalarField::New
    (
        IOobject::groupName("omega", this->alphaRhoPhi_.group()),
        IOobject::NO_REGISTER,
        this->mesh_,
        dimensionedScalar(dimless/dimTime, Zero)
    );
}


template<class BasicTurbulenceModel>
Foam::tmp<Foam::volSymmTensorField>
Foam::laminarThermalModel<BasicTurbulenceModel>::R() const
{
    return volSymmTensorField::New
    (
        IOobject::groupName("R", this->alphaRhoPhi_.group()),
        IOobject::NO_REGISTER,
        this->mesh_,
        dimensionedSymmTensor(sqr(this->U_.dimensions()), Zero)
    );
}


template<class BasicTurbulenceModel>
void Foam::laminarThermalModel<BasicTurbulenceModel>::correct()
{
    BasicTurbulenceModel::correct();
}


// ************************************************************************* //
