/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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

#include "faceMDLimitedGrad.H"
#include "cellMDLimitedGrad.H"
#include "gaussGrad.H"
#include "fvMesh.H"
#include "volMesh.H"
#include "surfaceMesh.H"
#include "volFields.H"
#include "fixedValueFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makeFvGradScheme(faceMDLimitedGrad)

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<>
Foam::tmp<Foam::volVectorField>
Foam::fv::faceMDLimitedGrad<Foam::scalar>::calcGrad
(
    const volScalarField& vsf,
    const word& name
) const
{
    const fvMesh& mesh = vsf.mesh();

    tmp<volVectorField> tGrad = basicGradScheme_().calcGrad(vsf, name);

    if (k_ < SMALL)
    {
        return tGrad;
    }

    volVectorField& g = tGrad.ref();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    const scalar rk = (1.0/k_ - 1.0);

    foamExecutor exec;
    const auto ownerPtr = owner.cbegin();
    const auto neighbourPtr = neighbour.cbegin();

    auto gPtr = g.begin();
    const auto vsfPtr = vsf.cbegin();
    const auto CPtr = C.cbegin();
    const auto CfPtr = Cf.cbegin();
    const scalar k = this->k_; 

    auto LambdaInitLimiter = [=](label facei)
    {
        const label own = ownerPtr[facei];
        const label nei = neighbourPtr[facei];

        const scalar vsfOwn = vsfPtr[own];
        const scalar vsfNei = vsfPtr[nei];

        scalar maxFace = max(vsfOwn, vsfNei);
        scalar minFace = min(vsfOwn, vsfNei);

        if (k < 1.0)
        {
            const scalar maxMinFace = rk*(maxFace - minFace);
            maxFace += maxMinFace;
            minFace -= maxMinFace;
        }

        // owner side
        cellMDLimitedGrad<scalar>::limitFace
        (
            gPtr[own],
            maxFace - vsfOwn,
            minFace - vsfOwn,
            CfPtr[facei] - CPtr[own]
        );

        // neighbour side
        cellMDLimitedGrad<scalar>::limitFace
        (
            gPtr[nei],
            maxFace - vsfNei,
            minFace - vsfNei,
            CfPtr[facei] - CPtr[nei]
        );
    };
    exec.parallelFor(LambdaInitLimiter, owner.size());

    const volScalarField::Boundary& bsf = vsf.boundaryField();

    forAll(bsf, patchi)
    {
        const fvPatchScalarField& psf = bsf[patchi];

        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
        const vectorField& pCf = Cf.boundaryField()[patchi];
       
        const auto psfPtr = psf.cbegin();
        const auto pOwnerPtr = pOwner.cbegin();
        const auto pCfPtr = pCf.cbegin();

        if (psf.coupled())
        {
            const scalarField psfNei(psf.patchNeighbourField());
            const auto psfNeiPtr = psfNei.cbegin();

            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];

                scalar vsfOwn = vsfPtr[own];
                scalar vsfNei = psfNeiPtr[pFacei];

                scalar maxFace = max(vsfOwn, vsfNei);
                scalar minFace = min(vsfOwn, vsfNei);

                if (k < 1.0)
                {
                    const scalar maxMinFace = rk*(maxFace - minFace);
                    maxFace += maxMinFace;
                    minFace -= maxMinFace;
                }

                cellMDLimitedGrad<scalar>::limitFace
                (
                    gPtr[own],
                    maxFace - vsfOwn,
                    minFace - vsfOwn,
                    pCfPtr[pFacei] - CPtr[own]
                );
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
        else if (psf.fixesValue())
        {
            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];

                const scalar vsfOwn = vsfPtr[own];
                const scalar vsfNei = psfPtr[pFacei];

                scalar maxFace = max(vsfOwn, vsfNei);
                scalar minFace = min(vsfOwn, vsfNei);

                if (k < 1.0)
                {
                    const scalar maxMinFace = rk*(maxFace - minFace);
                    maxFace += maxMinFace;
                    minFace -= maxMinFace;
                }

                cellMDLimitedGrad<scalar>::limitFace
                (
                    gPtr[own],
                    maxFace - vsfOwn,
                    minFace - vsfOwn,
                    pCfPtr[pFacei] - CPtr[own]
                );
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
    }

    g.correctBoundaryConditions();
    gaussGrad<scalar>::correctBoundaryConditions(vsf, g);

    return tGrad;
}


template<>
Foam::tmp<Foam::volTensorField>
Foam::fv::faceMDLimitedGrad<Foam::vector>::calcGrad
(
    const volVectorField& vvf,
    const word& name
) const
{
    const fvMesh& mesh = vvf.mesh();

    tmp<volTensorField> tGrad = basicGradScheme_().calcGrad(vvf, name);

    if (k_ < SMALL)
    {
        return tGrad;
    }

    volTensorField& g = tGrad.ref();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    const scalar rk = (1.0/k_ - 1.0);

    foamExecutor exec;
    const auto ownerPtr = owner.cbegin();
    const auto neighbourPtr = neighbour.cbegin();

    auto gPtr = g.begin();
    const auto vvfPtr = vvf.cbegin();
    const auto CPtr = C.cbegin();
    const auto CfPtr = Cf.cbegin();
    const scalar k = this->k_;

    auto LambdaInitLimiter = [=](label facei)
    {
        const label own = ownerPtr[facei];
        const label nei = neighbourPtr[facei];

        const vector& vvfOwn = vvfPtr[own];
        const vector& vvfNei = vvfPtr[nei];

        vector maxFace(max(vvfOwn, vvfNei));
        vector minFace(min(vvfOwn, vvfNei));

        if (k < 1.0)
        {
            const vector maxMinFace(rk*(maxFace - minFace));
            maxFace += maxMinFace;
            minFace -= maxMinFace;
        }

        // owner side
        cellMDLimitedGrad<vector>::limitFace
        (
            gPtr[own],
            maxFace - vvfOwn,
            minFace - vvfOwn,
            CfPtr[facei] - CPtr[own]
        );

        // neighbour side
        cellMDLimitedGrad<vector>::limitFace
        (
            gPtr[nei],
            maxFace - vvfNei,
            minFace - vvfNei,
            CfPtr[facei] - CPtr[nei]
        );
    };
    exec.parallelFor(LambdaInitLimiter, owner.size());

    const volVectorField::Boundary& bvf = vvf.boundaryField();

    forAll(bvf, patchi)
    {
        const fvPatchVectorField& psf = bvf[patchi];

        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
        const vectorField& pCf = Cf.boundaryField()[patchi];

        const auto psfPtr = psf.cbegin();
        const auto pOwnerPtr = pOwner.cbegin();
        const auto pCfPtr = pCf.cbegin();

        if (psf.coupled())
        {
            const vectorField psfNei(psf.patchNeighbourField());
            const auto psfNeiPtr = psfNei.cbegin();

            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];

                const vector& vvfOwn = vvfPtr[own];
                const vector& vvfNei = psfNeiPtr[pFacei];

                vector maxFace(max(vvfOwn, vvfNei));
                vector minFace(min(vvfOwn, vvfNei));

                if (k < 1.0)
                {
                    const vector maxMinFace(rk*(maxFace - minFace));
                    maxFace += maxMinFace;
                    minFace -= maxMinFace;
                }

                cellMDLimitedGrad<vector>::limitFace
                (
                    gPtr[own],
                    maxFace - vvfOwn, 
                    minFace - vvfOwn,
                    pCfPtr[pFacei] - CPtr[own]
                );
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
        else if (psf.fixesValue())
        {
            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];

                const vector& vvfOwn = vvfPtr[own];
                const vector& vvfNei = psfPtr[pFacei];

                vector maxFace(max(vvfOwn, vvfNei));
                vector minFace(min(vvfOwn, vvfNei));

                if (k < 1.0)
                {
                    const vector maxMinFace(rk*(maxFace - minFace));
                    maxFace += maxMinFace;
                    minFace -= maxMinFace;
                }

                cellMDLimitedGrad<vector>::limitFace
                (
                    gPtr[own],
                    maxFace - vvfOwn,
                    minFace - vvfOwn,
                    pCfPtr[pFacei] - CPtr[own]
                );
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
    }

    g.correctBoundaryConditions();
    gaussGrad<vector>::correctBoundaryConditions(vvf, g);

    return tGrad;
}


// ************************************************************************* //
