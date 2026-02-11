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

#include "cellMDLimitedGrad.H"
#include "gaussGrad.H"
#include "fvMesh.H"
#include "volMesh.H"
#include "surfaceMesh.H"
#include "volFields.H"
#include "fixedValueFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

makeFvGradScheme(cellMDLimitedGrad)

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<>
Foam::tmp<Foam::volVectorField>
Foam::fv::cellMDLimitedGrad<Foam::scalar>::calcGrad
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

    scalarField maxVsf(vsf.primitiveField());
    scalarField minVsf(vsf.primitiveField());

    foamExecutor exec;
    const auto ownerPtr = owner.cbegin();
    const auto neighbourPtr = neighbour.cbegin();

    auto gPtr = g.begin();
    const auto vsfPtr = vsf.cbegin();
    auto maxVsfPtr = maxVsf.begin();
    auto minVsfPtr = minVsf.begin();
    const auto CPtr = C.cbegin();
    const auto CfPtr = Cf.cbegin();

    auto Lambda = [=](label facei)
    {
        const label own = ownerPtr[facei];
        const label nei = neighbourPtr[facei];

        const scalar& vsfOwn = vsfPtr[own];
        const scalar& vsfNei = vsfPtr[nei];

        foamAtomic::AtomicMax(maxVsfPtr[own], vsfNei);
        foamAtomic::AtomicMin(minVsfPtr[own], vsfNei);

        foamAtomic::AtomicMax(maxVsfPtr[nei], vsfOwn);
        foamAtomic::AtomicMin(minVsfPtr[nei], vsfOwn);
    };
    exec.parallelFor(Lambda, owner.size());

    const volScalarField::Boundary& bsf = vsf.boundaryField();

    forAll(bsf, patchi)
    {
        const fvPatchScalarField& psf = bsf[patchi];
        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
        const auto psfPtr = psf.cbegin();
        const auto pOwnerPtr = pOwner.cbegin();

        if (psf.coupled())
        {
            const scalarField psfNei(psf.patchNeighbourField());
            const auto psfNeiPtr = psfNei.cbegin();
            
            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];
                const scalar& vsfNei = psfNeiPtr[pFacei];

                foamAtomic::AtomicMax(maxVsfPtr[own], vsfNei);
                foamAtomic::AtomicMin(minVsfPtr[own], vsfNei);
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
        else
        {
            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];
                const scalar& vsfNei = psfPtr[pFacei];

                foamAtomic::AtomicMax(maxVsfPtr[own], vsfNei);
                foamAtomic::AtomicMin(minVsfPtr[own], vsfNei);
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
    }

    maxVsf -= vsf;
    minVsf -= vsf;

    if (k_ < 1.0)
    {
        const scalarField maxMinVsf((1.0/k_ - 1.0)*(maxVsf - minVsf));
        maxVsf += maxMinVsf;
        minVsf -= maxMinVsf;
    }

    auto LambdaInitLimiter = [=](label facei)
    {
        const label own = ownerPtr[facei];
        const label nei = neighbourPtr[facei];

        // owner side
        limitFace
        (
            gPtr[own],
            maxVsfPtr[own],
            minVsfPtr[own],
            CfPtr[facei] - CPtr[own]
        );

        // neighbour side
        limitFace
        (
            gPtr[nei],
            maxVsfPtr[nei],
            minVsfPtr[nei],
            CfPtr[facei] - CPtr[nei]
        );
    };
    exec.parallelFor(LambdaInitLimiter, owner.size());

    forAll(bsf, patchi)
    {
        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
        const vectorField& pCf = Cf.boundaryField()[patchi];

        const auto pOwnerPtr = pOwner.cbegin();
        const auto pCfPtr = pCf.cbegin();

        auto Lambda = [=](label pFacei)
        {
            const label own = pOwnerPtr[pFacei];

            limitFace
            (
                gPtr[own],
                maxVsfPtr[own],
                minVsfPtr[own],
                pCfPtr[pFacei] - CPtr[own]
            );
        };
        exec.parallelFor(Lambda, pOwner.size());
    }

    g.correctBoundaryConditions();
    gaussGrad<scalar>::correctBoundaryConditions(vsf, g);

    return tGrad;
}


template<>
Foam::tmp<Foam::volTensorField>
Foam::fv::cellMDLimitedGrad<Foam::vector>::calcGrad
(
    const volVectorField& vsf,
    const word& name
) const
{
    const fvMesh& mesh = vsf.mesh();

    tmp<volTensorField> tGrad = basicGradScheme_().calcGrad(vsf, name);

    if (k_ < SMALL)
    {
        return tGrad;
    }

    volTensorField& g = tGrad.ref();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    vectorField maxVsf(vsf.primitiveField());
    vectorField minVsf(vsf.primitiveField());

    foamExecutor exec;
    const auto ownerPtr = owner.cbegin();
    const auto neighbourPtr = neighbour.cbegin();

    auto gPtr = g.begin();
    const auto vsfPtr = vsf.cbegin();
    auto maxVsfPtr = maxVsf.begin();
    auto minVsfPtr = minVsf.begin();
    const auto CPtr = C.cbegin();
    const auto CfPtr = Cf.cbegin();

    auto Lambda = [=](label facei)
    {
        const label own = ownerPtr[facei];
        const label nei = neighbourPtr[facei];

        const vector& vsfOwn = vsfPtr[own];
        const vector& vsfNei = vsfPtr[nei];

        foamAtomic::AtomicMax(maxVsfPtr[own], vsfNei);
        foamAtomic::AtomicMin(minVsfPtr[own], vsfNei);

        foamAtomic::AtomicMax(maxVsfPtr[nei], vsfOwn);
        foamAtomic::AtomicMin(minVsfPtr[nei], vsfOwn);
    };
    exec.parallelFor(Lambda, owner.size());

    const volVectorField::Boundary& bsf = vsf.boundaryField();

    forAll(bsf, patchi)
    {
        const fvPatchVectorField& psf = bsf[patchi];
        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
        const auto psfPtr = psf.cbegin();
        const auto pOwnerPtr = pOwner.cbegin();

        if (psf.coupled())
        {
            const vectorField psfNei(psf.patchNeighbourField());
            const auto psfNeiPtr = psfNei.cbegin();

            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];
                const vector& vsfNei = psfNeiPtr[pFacei];

                foamAtomic::AtomicMax(maxVsfPtr[own], vsfNei);
                foamAtomic::AtomicMin(minVsfPtr[own], vsfNei);
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
        else
        {
            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerPtr[pFacei];
                const vector& vsfNei = psfPtr[pFacei];

                foamAtomic::AtomicMax(maxVsfPtr[own], vsfNei);
                foamAtomic::AtomicMin(minVsfPtr[own], vsfNei);
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
    }

    maxVsf -= vsf;
    minVsf -= vsf;

    if (k_ < 1.0)
    {
        const vectorField maxMinVsf((1.0/k_ - 1.0)*(maxVsf - minVsf));
        maxVsf += maxMinVsf;
        minVsf -= maxMinVsf;
    }

    auto LambdaInitLimiter = [=](label facei)
    {
        const label own = ownerPtr[facei];
        const label nei = neighbourPtr[facei];

        // owner side
        limitFace
        (
            gPtr[own],
            maxVsfPtr[own],
            minVsfPtr[own],
            CfPtr[facei] - CPtr[own]
        );

        // neighbour side
        limitFace
        (
            gPtr[nei],
            maxVsfPtr[nei],
            minVsfPtr[nei],
            CfPtr[facei] - CPtr[nei]
        );
    };
    exec.parallelFor(LambdaInitLimiter, owner.size());

    forAll(bsf, patchi)
    {
        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
        const vectorField& pCf = Cf.boundaryField()[patchi];

        const auto pOwnerPtr = pOwner.cbegin();
        const auto pCfPtr = pCf.cbegin();

        auto Lambda = [=](label pFacei)
        {
            const label own = pOwnerPtr[pFacei];

            limitFace
            (
                gPtr[own],
                maxVsfPtr[own],
                minVsfPtr[own],
                pCfPtr[pFacei] - CPtr[own]
            );
        };
        exec.parallelFor(Lambda, pOwner.size());
    }

    g.correctBoundaryConditions();
    gaussGrad<vector>::correctBoundaryConditions(vsf, g);

    return tGrad;
}


// ************************************************************************* //
