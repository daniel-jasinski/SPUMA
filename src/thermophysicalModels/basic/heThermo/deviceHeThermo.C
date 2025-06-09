/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2015-2021 OpenCFD Ltd.
    Copyright (C) 2025 Cineca
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

#include "deviceHeThermo.H"
#include "gradientEnergyFvPatchScalarField.H"
#include "mixedEnergyFvPatchScalarField.H"

// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

template<class BasicThermo, class MixtureType>
void Foam::deviceHeThermo<BasicThermo, MixtureType>::
heBoundaryCorrection(volScalarField& h)
{
    volScalarField::Boundary& hBf = h.boundaryFieldRef();

    forAll(hBf, patchi)
    {
        if (isA<gradientEnergyFvPatchScalarField>(hBf[patchi]))
        {
            refCast<gradientEnergyFvPatchScalarField>(hBf[patchi]).gradient()
                = hBf[patchi].fvPatchField::snGrad();
        }
        else if (isA<mixedEnergyFvPatchScalarField>(hBf[patchi]))
        {
            refCast<mixedEnergyFvPatchScalarField>(hBf[patchi]).refGrad()
                = hBf[patchi].fvPatchField::snGrad();
        }
    }
}


template<class BasicThermo, class MixtureType>
void Foam::deviceHeThermo<BasicThermo, MixtureType>::init
(
    const volScalarField& p,
    const volScalarField& T,
    volScalarField& he
)
{
    scalarField& heCells = he.primitiveFieldRef();
    const scalarField& pCells = p.primitiveField();
    const scalarField& TCells = T.primitiveField();

    foamExecutor exec;
    const auto TCellsPtr = TCells.cbegin();
    const auto pCellsPtr = pCells.cbegin();
    auto heCellsPtr = heCells.begin();
    const MixtureType mixture(*this);
    
    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        heCellsPtr[celli] = cellMixture.HE(pCellsPtr[celli], TCellsPtr[celli]);
    };
    exec.parallelFor(Lambda, TCells.size());

    volScalarField::Boundary& heBf = he.boundaryFieldRef();

    forAll(heBf, patchi)
    {
        heBf[patchi] == this->he
        (
            p.boundaryField()[patchi],
            T.boundaryField()[patchi],
            patchi
        );

        heBf[patchi].useImplicit(T.boundaryField()[patchi].useImplicit());
    }

    this->heBoundaryCorrection(he);

    // Note: T does not have oldTime
    if (p.nOldTimes())
    {
        init(p.oldTime(), T.oldTime(), he.oldTime());
    }
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class BasicThermo, class MixtureType>
Foam::deviceHeThermo<BasicThermo, MixtureType>::deviceHeThermo
(
    const fvMesh& mesh,
    const word& phaseName
)
:
    BasicThermo(mesh, phaseName),
    MixtureType(*this, mesh, phaseName),

    he_
    (
        IOobject
        (
            BasicThermo::phasePropertyName
            (
                MixtureType::thermoType::heName()
            ),
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimEnergy/dimMass,
        this->heBoundaryTypes(),
        this->heBoundaryBaseTypes()
    )
{
    init(this->p_, this->T_, he_);
}


template<class BasicThermo, class MixtureType>
Foam::deviceHeThermo<BasicThermo, MixtureType>::deviceHeThermo
(
    const fvMesh& mesh,
    const dictionary& dict,
    const word& phaseName
)
:
    BasicThermo(mesh, dict, phaseName),
    MixtureType(*this, mesh, phaseName),

    he_
    (
        IOobject
        (
            BasicThermo::phasePropertyName
            (
                MixtureType::thermoType::heName()
            ),
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimEnergy/dimMass,
        this->heBoundaryTypes(),
        this->heBoundaryBaseTypes()
    )
{
    init(this->p_, this->T_, he_);
}


template<class BasicThermo, class MixtureType>
Foam::deviceHeThermo<BasicThermo, MixtureType>::deviceHeThermo
(
    const fvMesh& mesh,
    const word& phaseName,
    const word& dictionaryName
)
:
    BasicThermo(mesh, phaseName, dictionaryName),
    MixtureType(*this, mesh, phaseName),

    he_
    (
        IOobject
        (
            BasicThermo::phasePropertyName
            (
                MixtureType::thermoType::heName()
            ),
            mesh.time().timeName(),
            mesh,
            IOobject::NO_READ,
            IOobject::NO_WRITE
        ),
        mesh,
        dimEnergy/dimMass,
        this->heBoundaryTypes(),
        this->heBoundaryBaseTypes()
    )
{
    init(this->p_, this->T_, he_);
}



// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

template<class BasicThermo, class MixtureType>
Foam::deviceHeThermo<BasicThermo, MixtureType>::~deviceHeThermo()
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::he
(
    const volScalarField& p,
    const volScalarField& T
) const
{
    const fvMesh& mesh = this->T_.mesh();

    auto the = volScalarField::New
    (
        "he",
        IOobject::NO_REGISTER,
        mesh,
        he_.dimensions()
    );
    auto& he = the.ref();

    scalarField& heCells = he.primitiveFieldRef();
    const scalarField& pCells = p;
    const scalarField& TCells = T;

    foamExecutor exec;
    const auto TCellsPtr = TCells.cbegin();
    const auto pCellsPtr = pCells.cbegin();
    auto heCellsPtr = heCells.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture = 
            mixture.cellMixture(celli);
        heCellsPtr[celli] = cellMixture.HE(pCellsPtr[celli], TCellsPtr[celli]);
    };
    exec.parallelFor(Lambda, TCells.size());

    volScalarField::Boundary& heBf = he.boundaryFieldRef();

    forAll(heBf, patchi)
    {
        scalarField& hep = heBf[patchi];
        const scalarField& pp = p.boundaryField()[patchi];
        const scalarField& Tp = T.boundaryField()[patchi];
        const auto TpPtr = Tp.cbegin();
        const auto ppPtr = pp.cbegin();
        auto hepPtr= hep.begin();

        auto LambdaPatch = [=] (label facei)
        {
            const typename MixtureType::thermoType& pFaceMixture = 
                mixture.patchFaceMixture(patchi, facei);
            hepPtr[facei] = pFaceMixture.HE(ppPtr[facei], TpPtr[facei]);
        };
        exec.parallelFor(LambdaPatch, Tp.size());
    }

    return the;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::he
(
    const scalarField& p,
    const scalarField& T,
    const labelList& cells
) const
{
    auto the = tmp<scalarField>::New(T.size());
    auto& he = the.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto hePtr = he.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        hePtr[celli] = cellMixture.HE(pPtr[celli], TPtr[celli]);
    };
    exec.parallelFor(Lambda, T.size());

    return the;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::he
(
    const scalarField& p,
    const scalarField& T,
    const label patchi
) const
{
    auto the = tmp<scalarField>::New(T.size());
    auto& he = the.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto hePtr = he.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label facei)
    {
        const typename MixtureType::thermoType& pFaceMixture = 
            mixture.patchFaceMixture(patchi, facei);
        hePtr[facei] = pFaceMixture.HE(pPtr[facei], TPtr[facei]);
    };
    exec.parallelFor(Lambda, T.size());

    return the;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::hc() const
{
    const fvMesh& mesh = this->T_.mesh();

    auto thc = volScalarField::New
    (
        "hc",
        IOobject::NO_REGISTER,
        mesh,
        he_.dimensions()
    );
    auto& hcf = thc.ref();

    scalarField& hcCells = hcf.primitiveFieldRef();

    foamExecutor exec;
    auto hcCellsPtr = hcCells.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        hcCellsPtr[celli] = cellMixture.Hc();
    };
    exec.parallelFor(Lambda, hcCells.size());

    volScalarField::Boundary& hcfBf = hcf.boundaryFieldRef();

    forAll(hcfBf, patchi)
    {
        scalarField& hcp = hcfBf[patchi];
        auto hcpPtr = hcp.begin();

        auto LambdaPatch = [=] (label facei)
        {
            const typename MixtureType::thermoType& pFaceMixture =
                mixture.patchFaceMixture(patchi, facei);
            hcpPtr[facei] = pFaceMixture.Hc();
        };
        exec.parallelFor(LambdaPatch, hcp.size());
    }

    return thc;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::Cp
(
    const scalarField& p,
    const scalarField& T,
    const label patchi
) const
{
    auto tCp = tmp<scalarField>::New(T.size());
    auto& cp = tCp.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto cpPtr = cp.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label facei)
    {
        const typename MixtureType::thermoType& pFaceMixture =
            mixture.patchFaceMixture(patchi, facei);
        cpPtr[facei] = pFaceMixture.Cp(pPtr[facei], TPtr[facei]);
    };
    exec.parallelFor(Lambda, T.size());

    return tCp;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::Cp
(
    const scalarField& p,
    const scalarField& T,
    const labelList& cells
) const
{
    auto tCp = tmp<scalarField>::New(T.size());
    auto& Cp = tCp.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto CpPtr = Cp.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        CpPtr[celli] = cellMixture.Cp(pPtr[celli], TPtr[celli]);
    };
    exec.parallelFor(Lambda, T.size());

    return tCp;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::Cp() const
{
    const fvMesh& mesh = this->T_.mesh();

    auto tCp = volScalarField::New
    (
        "Cp",
        IOobject::NO_REGISTER,
        mesh,
        dimEnergy/dimMass/dimTemperature
    );
    auto& cp = tCp.ref();

    foamExecutor exec;
    const auto TCellsPtr = this->T_.cbegin();
    const auto pCellsPtr = this->p_.cbegin();
    auto cpPtr = cp.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        cpPtr[celli] = cellMixture.Cp(pCellsPtr[celli], TCellsPtr[celli]);
    };
    exec.parallelFor(Lambda, this->T_.size());

    volScalarField::Boundary& cpBf = cp.boundaryFieldRef();

    forAll(cpBf, patchi)
    {
        const fvPatchScalarField& pp = this->p_.boundaryField()[patchi];
        const fvPatchScalarField& pT = this->T_.boundaryField()[patchi];
        fvPatchScalarField& pCp = cpBf[patchi];
        
        const auto pTPtr = pT.cbegin();
        const auto ppPtr = pp.cbegin();
        auto pCpPtr = pCp.begin();

        auto LambdaPatch = [=] (label facei)
        {
            const typename MixtureType::thermoType& pFaceMixture =
                mixture.patchFaceMixture(patchi, facei);
            pCpPtr[facei] = pFaceMixture.Cp(ppPtr[facei], pTPtr[facei]);
        };
        exec.parallelFor(LambdaPatch, pT.size());
    }

    return tCp;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::Cv
(
    const scalarField& p,
    const scalarField& T,
    const label patchi
) const
{
    auto tCv = tmp<scalarField>::New(T.size());
    auto& cv = tCv.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto cvPtr = cv.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label facei)
    {
        const typename MixtureType::thermoType& pFaceMixture =
            mixture.patchFaceMixture(patchi, facei);
        cvPtr[facei] = pFaceMixture.Cv(pPtr[facei], TPtr[facei]);
    };
    exec.parallelFor(Lambda, T.size());

    return tCv;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::rhoEoS
(
    const scalarField& p,
    const scalarField& T,
    const labelList& cells
) const
{
    auto tRho = tmp<scalarField>::New(T.size());
    auto& rho = tRho.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto rhoPtr = rho.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        rhoPtr[celli] = cellMixture.rho(pPtr[celli], TPtr[celli]);
    };
    exec.parallelFor(Lambda, T.size());

    return tRho;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::Cv() const
{
    const fvMesh& mesh = this->T_.mesh();

    auto tCv = volScalarField::New
    (
        "Cv",
        IOobject::NO_REGISTER,
        mesh,
        dimEnergy/dimMass/dimTemperature
    );
    auto& cv = tCv.ref();

    foamExecutor exec;
    const auto TCellsPtr = this->T_.cbegin();
    const auto pCellsPtr = this->p_.cbegin();
    auto cvPtr = cv.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        cvPtr[celli] = cellMixture.Cv(pCellsPtr[celli], TCellsPtr[celli]);
    };
    exec.parallelFor(Lambda, this->T_.size());

    volScalarField::Boundary& cvBf = cv.boundaryFieldRef();

    forAll(cvBf, patchi)
    {
        cvBf[patchi] = Cv
        (
            this->p_.boundaryField()[patchi],
            this->T_.boundaryField()[patchi],
            patchi
        );
    }

    return tCv;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::gamma
(
    const scalarField& p,
    const scalarField& T,
    const label patchi
) const
{
    auto tgamma = tmp<scalarField>::New(T.size());
    auto& gamma = tgamma.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto gammaPtr = gamma.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label facei)
    {
        const typename MixtureType::thermoType& pFaceMixture =
            mixture.patchFaceMixture(patchi, facei);
        gammaPtr[facei] = pFaceMixture.gamma(pPtr[facei], TPtr[facei]);
    };
    exec.parallelFor(Lambda, T.size());

    return tgamma;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::gamma() const
{
    const fvMesh& mesh = this->T_.mesh();

    auto tgamma = volScalarField::New
    (
        "gamma",
        IOobject::NO_REGISTER,
        mesh,
        dimless
    );
    auto& gamma = tgamma.ref();

    foamExecutor exec;
    const auto TCellsPtr = this->T_.cbegin();
    const auto pCellsPtr = this->p_.cbegin();
    auto gammaPtr = gamma.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        gammaPtr[celli] = cellMixture.gamma(pCellsPtr[celli], TCellsPtr[celli]);
    };
    exec.parallelFor(Lambda, this->T_.size());

    volScalarField::Boundary& gammaBf = gamma.boundaryFieldRef();

    forAll(gammaBf, patchi)
    {
        const fvPatchScalarField& pp = this->p_.boundaryField()[patchi];
        const fvPatchScalarField& pT = this->T_.boundaryField()[patchi];
        fvPatchScalarField& pgamma = gammaBf[patchi];

        auto pgammaPtr = pgamma.begin();
        const auto ppPtr = pp.cbegin();
        const auto pTPtr = pT.cbegin();

        auto LambdaPatch = [=](label facei)
        {
            const typename MixtureType::thermoType& pFaceMixture =
                mixture.patchFaceMixture(patchi, facei);
            pgammaPtr[facei] = pFaceMixture.gamma
            (
                ppPtr[facei],
                pTPtr[facei]
            );
        };
        exec.parallelFor(LambdaPatch, pT.size());
    }

    return tgamma;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::Cpv
(
    const scalarField& p,
    const scalarField& T,
    const label patchi
) const
{
    auto tCpv = tmp<scalarField>::New(T.size());
    auto& Cpv = tCpv.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto CpvPtr = Cpv.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label facei)
    {
        const typename MixtureType::thermoType& pFaceMixture =
            mixture.patchFaceMixture(patchi, facei);
        CpvPtr[facei] = pFaceMixture.Cpv(pPtr[facei], TPtr[facei]);
    };
    exec.parallelFor(Lambda, T.size());

    return tCpv;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::Cpv() const
{
    const fvMesh& mesh = this->T_.mesh();

    auto tCpv = volScalarField::New
    (
        "Cpv",
        IOobject::NO_REGISTER,
        mesh,
        dimEnergy/dimMass/dimTemperature
    );
    auto& Cpv = tCpv.ref();

    foamExecutor exec;
    const auto TCellsPtr = this->T_.cbegin();
    const auto pCellsPtr = this->p_.cbegin();
    auto CpvPtr = Cpv.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        CpvPtr[celli] = cellMixture.Cpv(pCellsPtr[celli], TCellsPtr[celli]);
    };
    exec.parallelFor(Lambda, this->T_.size());

    volScalarField::Boundary& CpvBf = Cpv.boundaryFieldRef();

    forAll(CpvBf, patchi)
    {
        const fvPatchScalarField& pp = this->p_.boundaryField()[patchi];
        const fvPatchScalarField& pT = this->T_.boundaryField()[patchi];
        fvPatchScalarField& pCpv = CpvBf[patchi];
        
        const auto pTPtr = pT.cbegin();
        const auto ppPtr = pp.cbegin();
        auto pCpvPtr = pCpv.begin();

        auto LambdaPatch = [=] (label facei)
        {
            const typename MixtureType::thermoType& pFaceMixture =
                mixture.patchFaceMixture(patchi, facei);
            pCpvPtr[facei] = pFaceMixture.Cpv(ppPtr[facei], pTPtr[facei]);
        };
        exec.parallelFor(LambdaPatch, pT.size());
    }

    return tCpv;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::CpByCpv
(
    const scalarField& p,
    const scalarField& T,
    const label patchi
) const
{
    auto tCpByCpv = tmp<scalarField>::New(T.size());
    auto& CpByCpv = tCpByCpv.ref();

    foamExecutor exec;
    const auto TPtr = T.cbegin();
    const auto pPtr = p.cbegin();
    auto CpByCpvPtr = CpByCpv.begin();
    const MixtureType mixture(*this);

    auto LambdaPatch = [=] (label facei)
    {
        const typename MixtureType::thermoType& pFaceMixture =
            mixture.patchFaceMixture(patchi, facei);
        CpByCpvPtr[facei] = pFaceMixture.CpByCpv(pPtr[facei], TPtr[facei]);
    };
    exec.parallelFor(LambdaPatch, T.size());

    return tCpByCpv;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::CpByCpv() const
{
    const fvMesh& mesh = this->T_.mesh();

    auto tCpByCpv = volScalarField::New
    (
        "CpByCpv",
        IOobject::NO_REGISTER,
        mesh,
        dimless
    );
    auto& CpByCpv = tCpByCpv.ref();

    foamExecutor exec;
    const auto TCellsPtr = this->T_.cbegin();
    const auto pCellsPtr = this->p_.cbegin();
    auto CpByCpvPtr = CpByCpv.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        CpByCpvPtr[celli] = cellMixture.CpByCpv(pCellsPtr[celli], TCellsPtr[celli]);
    };
    exec.parallelFor(Lambda, this->T_.size());

    volScalarField::Boundary& CpByCpvBf =
        CpByCpv.boundaryFieldRef();

    forAll(CpByCpvBf, patchi)
    {
        const fvPatchScalarField& pp = this->p_.boundaryField()[patchi];
        const fvPatchScalarField& pT = this->T_.boundaryField()[patchi];
        fvPatchScalarField& pCpByCpv = CpByCpvBf[patchi];
        
        const auto pTPtr = pT.cbegin();
        const auto ppPtr = pp.cbegin();
        auto pCpByCpvPtr = pCpByCpv.begin();

        auto LambdaPatch = [=] (label facei)
        {
            const typename MixtureType::thermoType& pFaceMixture =
                mixture.patchFaceMixture(patchi, facei);
            pCpByCpvPtr[facei] = pFaceMixture.CpByCpv(ppPtr[facei], pTPtr[facei]);
        };
        exec.parallelFor(LambdaPatch, pT.size());
    }

    return tCpByCpv;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::THE
(
    const scalarField& h,
    const scalarField& p,
    const scalarField& T0,
    const labelList& cells
) const
{
    auto tT = tmp<scalarField>::New(h.size());
    auto& T = tT.ref();

    foamExecutor exec;
    const auto T0Ptr = T0.cbegin();
    const auto pPtr = p.cbegin();
    const auto hPtr = h.cbegin();
    auto TPtr = T.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        TPtr[celli] = cellMixture.THE(hPtr[celli], pPtr[celli], T0Ptr[celli]);
    };
    exec.parallelFor(Lambda, h.size());

    return tT;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::THE
(
    const scalarField& h,
    const scalarField& p,
    const scalarField& T0,
    const label patchi
) const
{

    auto tT = tmp<scalarField>::New(h.size());
    auto& T = tT.ref();

    foamExecutor exec;
    const auto T0Ptr = T0.cbegin();
    const auto pPtr = p.cbegin();
    const auto hPtr = h.cbegin();
    auto TPtr = T.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label facei)
    {
        const typename MixtureType::thermoType& pFaceMixture =
            mixture.patchFaceMixture(patchi, facei);
        TPtr[facei] = pFaceMixture.THE(hPtr[facei], pPtr[facei], T0Ptr[facei]);
    };
    exec.parallelFor(Lambda, h.size());

    return tT;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::W
(
) const
{
    const fvMesh& mesh = this->T_.mesh();

    auto tW = volScalarField::New
    (
        "W",
        IOobject::NO_REGISTER,
        mesh,
        dimMass/dimMoles
    );
    auto& W = tW.ref();

    scalarField& WCells = W.primitiveFieldRef();

    foamExecutor exec;
    auto WCellsPtr = WCells.begin();
    const MixtureType mixture(*this);

    auto Lambda = [=] (label celli)
    {
        const typename MixtureType::thermoType& cellMixture =
            mixture.cellMixture(celli);
        WCellsPtr[celli] = cellMixture.W();
    };
    exec.parallelFor(Lambda, WCells.size());

    auto& WBf = W.boundaryFieldRef();

    forAll(WBf, patchi)
    {
        scalarField& Wp = WBf[patchi];
        auto WpPtr = Wp.begin();

        auto LambdaPatch = [=] (label facei)
        {
            const typename MixtureType::thermoType& pFaceMixture =
                mixture.patchFaceMixture(patchi, facei);
            WpPtr[facei] = pFaceMixture.W();
        };
        exec.parallelFor(LambdaPatch, Wp.size());
    }

    return tW;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::kappa() const
{
    tmp<Foam::volScalarField> kappa(Cp()*this->alpha_);
    kappa.ref().rename("kappa");
    return kappa;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField> Foam::deviceHeThermo<BasicThermo, MixtureType>::kappa
(
    const label patchi
) const
{
    return
        Cp
        (
            this->p_.boundaryField()[patchi],
            this->T_.boundaryField()[patchi],
            patchi
        )*this->alpha_.boundaryField()[patchi];
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::alphahe() const
{
    tmp<Foam::volScalarField> alphaEff(this->CpByCpv()*this->alpha_);
    alphaEff.ref().rename("alphahe");
    return alphaEff;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::alphahe(const label patchi) const
{
    return
    this->CpByCpv
    (
        this->p_.boundaryField()[patchi],
        this->T_.boundaryField()[patchi],
        patchi
    )
   *this->alpha_.boundaryField()[patchi];
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::kappaEff
(
    const volScalarField& alphat
) const
{
    tmp<Foam::volScalarField> kappaEff(Cp()*(this->alpha_ + alphat));
    kappaEff.ref().rename("kappaEff");
    return kappaEff;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::kappaEff
(
    const scalarField& alphat,
    const label patchi
) const
{
    return
        Cp
        (
            this->p_.boundaryField()[patchi],
            this->T_.boundaryField()[patchi],
            patchi
        )
       *(
           this->alpha_.boundaryField()[patchi]
         + alphat
        );
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::volScalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::alphaEff
(
    const volScalarField& alphat
) const
{
    tmp<Foam::volScalarField> alphaEff(this->CpByCpv()*(this->alpha_ + alphat));
    alphaEff.ref().rename("alphaEff");
    return alphaEff;
}


template<class BasicThermo, class MixtureType>
Foam::tmp<Foam::scalarField>
Foam::deviceHeThermo<BasicThermo, MixtureType>::alphaEff
(
    const scalarField& alphat,
    const label patchi
) const
{
    return
    this->CpByCpv
    (
        this->p_.boundaryField()[patchi],
        this->T_.boundaryField()[patchi],
        patchi
    )
   *(
        this->alpha_.boundaryField()[patchi]
      + alphat
    );
}


template<class BasicThermo, class MixtureType>
bool Foam::deviceHeThermo<BasicThermo, MixtureType>::read()
{
    if (BasicThermo::read())
    {
        MixtureType::read(*this);
        return true;
    }

    return false;
}


// ************************************************************************* //
