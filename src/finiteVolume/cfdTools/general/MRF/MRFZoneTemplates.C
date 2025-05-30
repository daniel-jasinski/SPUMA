/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2018 OpenFOAM Foundation
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

#include "MRFZone.H"
#include "fvMesh.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "fvMatrices.H"
#include "deviceUtils.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class RhoFieldType>
void Foam::MRFZone::makeRelativeRhoFlux
(
    const RhoFieldType& rho,
    surfaceScalarField& phi
) const
{
    if (!active_)
    {
        return;
    }

    const surfaceVectorField& Cf = mesh_.Cf();
    const surfaceVectorField& Sf = mesh_.Sf();

    const vector Omega = omega_->value(mesh_.time().timeOutputValue())*axis_;

    const vectorField& Cfi = Cf;
    const vectorField& Sfi = Sf;
    scalarField& phii = phi.primitiveFieldRef();

    foamExecutor exec;
    auto phiiPtr = phii.begin();
    const auto iFacesPtr = internalFaces_.cbegin();
    const auto CfiPtr = Cfi.cbegin();
    const auto SfiPtr = Sfi.cbegin();
    const auto rhoPtr = argWrapper::cget(rho);
    const vector local_origin(origin_);
    // Internal faces
    auto Lambda = [=](label i)
    {
        label facei = iFacesPtr[i];
        phiiPtr[facei] -= rhoPtr[facei]*(Omega ^ (CfiPtr[facei] - local_origin)) & SfiPtr[facei];
    };
    exec.parallelFor(Lambda, internalFaces_.size());

    makeRelativeRhoFlux(rho.boundaryField(), phi.boundaryFieldRef());
}


template<class RhoFieldType>
void Foam::MRFZone::makeRelativeRhoFlux
(
    const RhoFieldType& rho,
    FieldField<fvsPatchField, scalar>& phi
) const
{
    if (!active_)
    {
        return;
    }

    const surfaceVectorField& Cf = mesh_.Cf();
    const surfaceVectorField& Sf = mesh_.Sf();

    const vector Omega = omega_->value(mesh_.time().timeOutputValue())*axis_;

    foamExecutor exec;

    // Included patches
    forAll(includedFaces_, patchi)
    {
        auto phiPtr = phi[patchi].begin();
        const auto incFacesPtr = includedFaces_[patchi].cbegin();
        // forAll(includedFaces_[patchi], i)
        // {
        //     label patchFacei = includedFaces_[patchi][i];

        //     phi[patchi][patchFacei] = 0.0;
        // }
        auto Lambda = [=](label i)
        {
            label patchFacei = incFacesPtr[i];

            phiPtr[patchFacei] = 0.0;
        };
        exec.parallelFor(Lambda, includedFaces_[patchi].size());
    }

    // Excluded patches
    const vector local_origin(origin_);
    forAll(excludedFaces_, patchi)
    {
        auto phiPtr = phi[patchi].begin();
        const auto exclFacesPtr = excludedFaces_[patchi].cbegin();
        const auto CfPtr = Cf.boundaryField()[patchi].cbegin();
        const auto SfPtr = Sf.boundaryField()[patchi].cbegin();
        const auto rhoPtr = argWrapper::cget(rho[patchi]);
        // forAll(excludedFaces_[patchi], i)
        // {
        //     label patchFacei = excludedFaces_[patchi][i];
        //     phi[patchi][patchFacei] -=
        //         rho[patchi][patchFacei]
        //       * (Omega ^ (Cf.boundaryField()[patchi][patchFacei] - origin_))
        //       & Sf.boundaryField()[patchi][patchFacei];
        // }
        auto Lambda = [=](label i)
        {
            label patchFacei = exclFacesPtr[i];

            phiPtr[patchFacei] -=
                rhoPtr[patchFacei]
                * (Omega ^ (CfPtr[patchFacei] - local_origin))
                & SfPtr[patchFacei];
        };
        exec.parallelFor(Lambda, excludedFaces_[patchi].size());

    }
}


template<class RhoFieldType>
void Foam::MRFZone::makeRelativeRhoFlux
(
    const RhoFieldType& rho,
    Field<scalar>& phi,
    const label patchi
) const
{
    if (!active_)
    {
        return;
    }

    const surfaceVectorField& Cf = mesh_.Cf();
    const surfaceVectorField& Sf = mesh_.Sf();

    const vector Omega = omega_->value(mesh_.time().timeOutputValue())*axis_;

    foamExecutor exec;
    auto phiPtr = phi.begin();
    const auto incFacesPtr = includedFaces_[patchi].cbegin();
    const auto exclFacesPtr = excludedFaces_[patchi].cbegin();
    const auto CfPtr = Cf.boundaryField()[patchi].cbegin();
    const auto SfPtr = Sf.boundaryField()[patchi].cbegin();
    const auto rhoPtr = argWrapper::cget(rho);
    const vector local_origin(origin_);
    // Included patches
    auto Lambda_i = [=](label i)
    {
        label patchFacei = incFacesPtr[i];

        phiPtr[patchFacei] = 0.0;
    };
    exec.parallelFor(Lambda_i, includedFaces_[patchi].size());

    // Excluded patches

    auto Lambda_e = [=](label i)
    {
        label patchFacei = exclFacesPtr[i];

        phiPtr[patchFacei] -=
            rhoPtr[patchFacei]
          * (Omega ^ (CfPtr[patchFacei] - local_origin))
          & SfPtr[patchFacei];
    };
    exec.parallelFor(Lambda_e, excludedFaces_[patchi].size());
}


template<class RhoFieldType>
void Foam::MRFZone::makeAbsoluteRhoFlux
(
    const RhoFieldType& rho,
    surfaceScalarField& phi
) const
{
    if (!active_)
    {
        return;
    }

    const surfaceVectorField& Cf = mesh_.Cf();
    const surfaceVectorField& Sf = mesh_.Sf();

    const vector Omega = omega_->value(mesh_.time().timeOutputValue())*axis_;

    const vectorField& Cfi = Cf;
    const vectorField& Sfi = Sf;
    scalarField& phii = phi.primitiveFieldRef();

    foamExecutor exec;
    auto phiiPtr = phii.begin();
    const auto iFacesPtr = internalFaces_.cbegin();
    const auto CfiPtr = Cfi.cbegin();
    const auto SfiPtr = Sfi.cbegin();
    const auto rhoPtr = argWrapper::cget(rho);
    const vector local_origin(origin_);

    //Internal faces
    auto Lambda = [=](label i)
    {
        label facei = iFacesPtr[i];
        phiiPtr[facei] += rhoPtr[facei]*(Omega ^ (CfiPtr[facei] - local_origin)) & SfiPtr[facei];
    };
    exec.parallelFor(Lambda, internalFaces_.size());

    surfaceScalarField::Boundary& phibf = phi.boundaryFieldRef();


    // Included patches
    forAll(includedFaces_, patchi)
    {
        auto phibfPtr = phibf[patchi].begin();
        const auto incFacesPtr = includedFaces_[patchi].cbegin();
        const auto rhoPtr = argWrapper::cget(rho.boundaryField()[patchi]);
        const auto CfPtr = Cf.boundaryField()[patchi].cbegin();
        const auto SfPtr = Sf.boundaryField()[patchi].cbegin();

        auto Lambda = [=](label i)
        {
            label patchFacei = incFacesPtr[i];

            phibfPtr[patchFacei] +=
                rhoPtr[patchFacei]
              * (Omega ^ (CfPtr[patchFacei] - local_origin))
              & SfPtr[patchFacei];
        };
        exec.parallelFor(Lambda, includedFaces_[patchi].size());
    }

    // Excluded patches
    forAll(excludedFaces_, patchi)
    {
        auto phibfPtr = phibf[patchi].begin();
        const auto exclFacesPtr = excludedFaces_[patchi].cbegin();
        const auto rhoPtr = argWrapper::cget(rho.boundaryField()[patchi]);
        const auto CfPtr = Cf.boundaryField()[patchi].cbegin();
        const auto SfPtr = Sf.boundaryField()[patchi].cbegin();

        auto Lambda = [=](label i)
        {
            label patchFacei = exclFacesPtr[i];

            phibfPtr[patchFacei] +=
                rhoPtr[patchFacei]
              * (Omega ^ (CfPtr[patchFacei] - local_origin))
              & SfPtr[patchFacei];
        };
        exec.parallelFor(Lambda, excludedFaces_[patchi].size());
    }
}


template<class Type>
void Foam::MRFZone::zero
(
    GeometricField<Type, fvsPatchField, surfaceMesh>& phi
) const
{
    if (!active_)
    {
        return;
    }

    Field<Type>& phii = phi.primitiveFieldRef();

    foamExecutor exec;
    auto phiiPtr = phii.begin();
    const auto iFacesPtr = internalFaces_.cbegin();

    auto Lambda = [=](label i)
    {
        phiiPtr[iFacesPtr[i]] = Zero;
    };
    exec.parallelFor(Lambda, internalFaces_.size());

    auto& phibf = phi.boundaryFieldRef();

    forAll(includedFaces_, patchi)
    {
        auto phibfPtr = phibf[patchi].begin();
        const auto incFacesPtr = includedFaces_[patchi].cbegin();

        auto Lambda = [=](label i)
        {
            phibfPtr[incFacesPtr[i]] = Zero;
        };
        exec.parallelFor(Lambda, includedFaces_[patchi].size());
    }

    forAll(excludedFaces_, patchi)
    {
        auto phibfPtr = phibf[patchi].begin();
        const auto exclFacesPtr = excludedFaces_[patchi].cbegin();

        auto Lambda = [=](label i)
        {
            phibfPtr[exclFacesPtr[i]] = Zero;
        };
        exec.parallelFor(Lambda, excludedFaces_[patchi].size());
    }
}


// ************************************************************************* //
