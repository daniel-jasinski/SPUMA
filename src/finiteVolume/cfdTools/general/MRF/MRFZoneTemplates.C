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
    auto phii_p = phii.begin();
    const auto iFaces_p = internalFaces_.cbegin();
    const auto Cfi_p = Cfi.cbegin();
    const auto Sfi_p = Sfi.cbegin();
    const auto rho_p = argWrapper::cget(rho);
    const vector local_origin(origin_);
    // Internal faces
    auto Lambda = [=](label i){
        label facei = iFaces_p[i];
        phii_p[facei] -= rho_p[facei]*(Omega ^ (Cfi_p[facei] - local_origin)) & Sfi_p[facei];
    };
    exec.parallelFor(Lambda,internalFaces_.size());

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
        auto phi_p = phi[patchi].begin();
        const auto incFaces_p = includedFaces_[patchi].cbegin();
        // forAll(includedFaces_[patchi], i)
        // {
        //     label patchFacei = includedFaces_[patchi][i];

        //     phi[patchi][patchFacei] = 0.0;
        // }
        auto Lambda = [=](label i){
            label patchFacei = incFaces_p[i];

            phi_p[patchFacei] = 0.0;
        };
        exec.parallelFor(Lambda,includedFaces_[patchi].size());
    }

    // Excluded patches
    const vector local_origin(origin_);
    forAll(excludedFaces_, patchi)
    {
        auto phi_p = phi[patchi].begin();
        const auto exclFaces_p = excludedFaces_[patchi].cbegin();
        const auto Cf_p = Cf.boundaryField()[patchi].cbegin();
        const auto Sf_p = Sf.boundaryField()[patchi].cbegin();
        const auto rho_p = argWrapper::cget(rho[patchi]);
        // forAll(excludedFaces_[patchi], i)
        // {
        //     label patchFacei = excludedFaces_[patchi][i];
        //     phi[patchi][patchFacei] -=
        //         rho[patchi][patchFacei]
        //       * (Omega ^ (Cf.boundaryField()[patchi][patchFacei] - origin_))
        //       & Sf.boundaryField()[patchi][patchFacei];
        // }
        auto Lambda = [=](label i){
            label patchFacei = exclFaces_p[i];

            phi_p[patchFacei] -=
                rho_p[patchFacei]
                * (Omega ^ (Cf_p[patchFacei] - local_origin))
                & Sf_p[patchFacei];
        };
        exec.parallelFor(Lambda,excludedFaces_[patchi].size());

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
    auto phi_p = phi.begin();
    const auto incFaces_p = includedFaces_[patchi].cbegin();
    const auto exclFaces_p = excludedFaces_[patchi].cbegin();
    const auto Cf_p = Cf.boundaryField()[patchi].cbegin(); 
    const auto Sf_p = Sf.boundaryField()[patchi].cbegin(); 
    const auto rho_p = argWrapper::cget(rho);
    const vector local_origin(origin_);
    // Included patches
    auto Lambda_i = [=](label i){
        label patchFacei = incFaces_p[i];

        phi_p[patchFacei] = 0.0;
    };
    exec.parallelFor(Lambda_i,includedFaces_[patchi].size());

    // Excluded patches

    auto Lambda_e = [=](label i){
        label patchFacei = exclFaces_p[i];

        phi_p[patchFacei] -=
            rho_p[patchFacei]
          * (Omega ^ (Cf_p[patchFacei] - local_origin))
          & Sf_p[patchFacei];
    };
    exec.parallelFor(Lambda_e,excludedFaces_[patchi].size());
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
    auto phii_p = phii.begin();
    const auto iFaces_p = internalFaces_.cbegin();
    const auto Cfi_p = Cfi.cbegin();
    const auto Sfi_p = Sfi.cbegin();
    const auto rho_p = argWrapper::cget(rho);
    const vector local_origin(origin_);

    //Internal faces
    auto Lambda = [=](label i){
        label facei = iFaces_p[i];
        phii_p[facei] += rho_p[facei]*(Omega ^ (Cfi_p[facei] - local_origin)) & Sfi_p[facei];
    };
    exec.parallelFor(Lambda,internalFaces_.size());

    surfaceScalarField::Boundary& phibf = phi.boundaryFieldRef();


    // Included patches
    forAll(includedFaces_, patchi)
    {
        auto phibf_p = phibf[patchi].begin();
        const auto incFaces_p = includedFaces_[patchi].cbegin();
        const auto rho_p = argWrapper::cget(rho.boundaryField()[patchi]);
        const auto Cf_p = Cf.boundaryField()[patchi].cbegin();
        const auto Sf_p = Sf.boundaryField()[patchi].cbegin();

        auto Lambda = [=](label i){
            label patchFacei = incFaces_p[i];

            phibf_p[patchFacei] +=
                rho_p[patchFacei]
              * (Omega ^ (Cf_p[patchFacei] - local_origin))
              & Sf_p[patchFacei];
        };
        exec.parallelFor(Lambda,includedFaces_[patchi].size());
    }

    // Excluded patches
    forAll(excludedFaces_, patchi)
    {
        auto phibf_p = phibf[patchi].begin();
        const auto exclFaces_p = excludedFaces_[patchi].cbegin();
        const auto rho_p = argWrapper::cget(rho.boundaryField()[patchi]);
        const auto Cf_p = Cf.boundaryField()[patchi].cbegin();
        const auto Sf_p = Sf.boundaryField()[patchi].cbegin();

        auto Lambda = [=](label i){
            label patchFacei = exclFaces_p[i];

            phibf_p[patchFacei] +=
                rho_p[patchFacei]
              * (Omega ^ (Cf_p[patchFacei] - local_origin))
              & Sf_p[patchFacei];
        };
        exec.parallelFor(Lambda,excludedFaces_[patchi].size());
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
    auto phii_p = phii.begin();
    const auto iFaces_p = internalFaces_.cbegin();

    auto Lambda = [=](label i){
        phii_p[iFaces_p[i]] = Zero;
    };
    exec.parallelFor(Lambda,internalFaces_.size());

    auto& phibf = phi.boundaryFieldRef();

    forAll(includedFaces_, patchi)
    {
        auto phibf_p = phibf[patchi].begin();
        const auto incFaces_p = includedFaces_[patchi].cbegin();

        auto Lambda = [=](label i){
            phibf_p[incFaces_p[i]] = Zero;
        };
        exec.parallelFor(Lambda,includedFaces_[patchi].size());
    }

    forAll(excludedFaces_, patchi)
    {
        auto phibf_p = phibf[patchi].begin();
        const auto exclFaces_p = excludedFaces_[patchi].cbegin();

        auto Lambda = [=](label i){
            phibf_p[exclFaces_p[i]] = Zero;
        };
        exec.parallelFor(Lambda,excludedFaces_[patchi].size());
    }
}


// ************************************************************************* //
