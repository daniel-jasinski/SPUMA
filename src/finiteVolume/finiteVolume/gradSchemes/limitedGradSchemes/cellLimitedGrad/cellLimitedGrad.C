/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2018 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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

#include "cellLimitedGrad.H"
#include "gaussGrad.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type, class Limiter>
void Foam::fv::cellLimitedGrad<Type, Limiter>::limitGradient
(
    const Field<scalar>& limiter,
    Field<vector>& gIf
) const
{
    gIf *= limiter;
}


template<class Type, class Limiter>
void Foam::fv::cellLimitedGrad<Type, Limiter>::limitGradient
(
    const Field<vector>& limiter,
    Field<tensor>& gIf
) const
{
    foamExecutor exec;
    auto gIfp = gIf.begin();
    auto limiterp = limiter.cbegin();
    auto Lambda = [=](label celli)
    {
        gIfp[celli] = tensor
        (
            cmptMultiply(limiterp[celli], gIfp[celli].x()),
            cmptMultiply(limiterp[celli], gIfp[celli].y()),
            cmptMultiply(limiterp[celli], gIfp[celli].z())
        );
    };
    exec.parallelFor(Lambda, gIf.size());
}


template<class Type, class Limiter>
Foam::tmp
<
    Foam::GeometricField
    <
        typename Foam::outerProduct<Foam::vector, Type>::type,
        Foam::fvPatchField,
        Foam::volMesh
    >
>
Foam::fv::cellLimitedGrad<Type, Limiter>::calcGrad
(
    const GeometricField<Type, fvPatchField, volMesh>& vsf,
    const word& name
) const
{
    const fvMesh& mesh = vsf.mesh();

    tmp
    <
        GeometricField
        <typename outerProduct<vector, Type>::type, fvPatchField, volMesh>
    > tGrad = basicGradScheme_().calcGrad(vsf, name);

    if (k_ < SMALL)
    {
        return tGrad;
    }

    GeometricField
    <
        typename outerProduct<vector, Type>::type,
        fvPatchField,
        volMesh
    >& g = tGrad.ref();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    Field<Type> maxVsf(vsf.primitiveField());
    Field<Type> minVsf(vsf.primitiveField());

    foamExecutor exec;
    const auto ownerp = owner.cbegin();
    const auto neighbourp = neighbour.cbegin();

    const auto vsfp = vsf.cbegin();
    auto maxVsfp = maxVsf.begin();
    auto minVsfp = minVsf.begin();
    const auto Cp = C.cbegin();
    const auto Cfp = Cf.cbegin();
    const auto gp = g.cbegin();

    auto Lambda = [=](label facei)
    {
        const label own = ownerp[facei];
        const label nei = neighbourp[facei];

        const Type& vsfOwn = vsfp[own];
        const Type& vsfNei = vsfp[nei];

        foamAtomic::AtomicMax(maxVsfp[own], vsfNei);
        foamAtomic::AtomicMin(minVsfp[own], vsfNei);

        foamAtomic::AtomicMax(maxVsfp[nei], vsfOwn);
        foamAtomic::AtomicMin(minVsfp[nei], vsfOwn);
    };
    exec.parallelFor(Lambda, owner.size());

    const auto& bsf = vsf.boundaryField();

    forAll(bsf, patchi)
    {
        const fvPatchField<Type>& psf = bsf[patchi];
        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();

        const auto psfp = psf.cbegin();
        const auto pOwnerp = pOwner.cbegin();

        if (psf.coupled())
        {
            const Field<Type> psfNei(psf.patchNeighbourField());
            const auto psfNeip = psfNei.cbegin();
            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerp[pFacei];
                const Type& vsfNei = psfNeip[pFacei];

                foamAtomic::AtomicMax(maxVsfp[own], vsfNei);
                foamAtomic::AtomicMin(minVsfp[own], vsfNei);
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
        else
        {
            auto Lambda = [=](label pFacei)
            {
                const label own = pOwnerp[pFacei];
                const Type& vsfNei = psfp[pFacei];

                foamAtomic::AtomicMax(maxVsfp[own], vsfNei);
                foamAtomic::AtomicMin(minVsfp[own], vsfNei);
            };
            exec.parallelFor(Lambda, pOwner.size());
        }
    }

    maxVsf -= vsf;
    minVsf -= vsf;

    if (k_ < 1.0)
    {
        const Field<Type> maxMinVsf((1.0/k_ - 1.0)*(maxVsf - minVsf));
        maxVsf += maxMinVsf;
        minVsf -= maxMinVsf;
    }

    // Create limiter initialized to 1
    // Note: the limiter is not permitted to be > 1
    Field<Type> limiter(vsf.primitiveField().size(), pTraits<Type>::one);
    auto limiterp = limiter.begin();
    Limiter localLimiter(*this);
    const label nComponents = pTraits<Type>::nComponents;

    auto LambdaInitLimiter = [=](label facei)
    {
        const label own = ownerp[facei];
        const label nei = neighbourp[facei];

        // owner side
        limitFace
        (
            limiterp[own],
            maxVsfp[own],
            minVsfp[own],
            (Cfp[facei] - Cp[own]) & gp[own],
            localLimiter,
            nComponents
        );

        // neighbour side
        limitFace
        (
            limiterp[nei],
            maxVsfp[nei],
            minVsfp[nei],
            (Cfp[facei] - Cp[nei]) & gp[nei],
            localLimiter,
            nComponents
        );
    };
    exec.parallelFor(LambdaInitLimiter, owner.size());

    forAll(bsf, patchi)
    {
        const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
        const vectorField& pCf = Cf.boundaryField()[patchi];

        const auto pOwnerp = pOwner.cbegin();
        const auto pCfp = pCf.cbegin();

        auto Lambda = [=](label pFacei)
        {
            const label own = pOwnerp[pFacei];

            limitFace
            (
                limiterp[own],
                maxVsfp[own],
                minVsfp[own],
                ((pCfp[pFacei] - Cp[own]) & gp[own]),
                localLimiter,
                nComponents
            );
        };
        exec.parallelFor(Lambda, pOwner.size());
    }

    if (fv::debug_())
    {
        Info<< "gradient limiter for: " << vsf.name()
            << " max = " << gMax(limiter)
            << " min = " << gMin(limiter)
            << " average: " << gAverage(limiter) << endl;
    }

    limitGradient(limiter, g);
    g.correctBoundaryConditions();
    gaussGrad<Type>::correctBoundaryConditions(vsf, g);

    return tGrad;
}


// ************************************************************************* //
