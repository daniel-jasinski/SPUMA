/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

#include "linearUpwindV.H"
#include "fvMesh.H"
#include "volFields.H"
#include "surfaceFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::linearUpwindV<Type>::correction
(
    const GeometricField<Type, fvPatchField, volMesh>& vf
) const
{
    const fvMesh& mesh = this->mesh();

    tmp<GeometricField<Type, fvsPatchField, surfaceMesh>> tsfCorr
    (
        new GeometricField<Type, fvsPatchField, surfaceMesh>
        (
            IOobject
            (
                "linearUpwindV::correction(" + vf.name() + ')',
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            ),
            mesh,
            dimensioned<Type>(vf.dimensions(), Zero)
        )
    );

    GeometricField<Type, fvsPatchField, surfaceMesh>& sfCorr = tsfCorr.ref();

    const surfaceScalarField& faceFlux = this->faceFlux_;
    const surfaceScalarField& w = mesh.weights();

    const labelList& own = mesh.owner();
    const labelList& nei = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    tmp
    <
        GeometricField
        <
            typename outerProduct<vector, Type>::type,
            fvPatchField,
            volMesh
        >
    > tgradVf = gradScheme_().grad(vf, gradSchemeName_);

    const GeometricField
    <
        typename outerProduct<vector, Type>::type,
        fvPatchField,
        volMesh
    >& gradVf = tgradVf();

    foamExecutor exec;
    auto sfCorrPtr = sfCorr.begin();
    const auto gradVfPtr = gradVf.cbegin();
    const auto CPtr = C.cbegin();
    const auto CfPtr = Cf.cbegin();
    const auto ownPtr = own.cbegin();
    const auto neiPtr = nei.cbegin();
    const auto wPtr = w.cbegin();
    const auto faceFluxPtr = faceFlux.cbegin();
    const auto vfPtr = vf.cbegin();

    auto Lambda = [=](label facei)
    {
        vector maxCorr;

        if (faceFluxPtr[facei] > 0.0)
        {
            maxCorr =
                (1.0 - wPtr[facei])*(vfPtr[neiPtr[facei]] - vfPtr[ownPtr[facei]]);

            sfCorrPtr[facei] =
                (CfPtr[facei] - CPtr[ownPtr[facei]]) & gradVfPtr[ownPtr[facei]];
        }
        else
        {
            maxCorr =
                wPtr[facei]*(vfPtr[ownPtr[facei]] - vfPtr[neiPtr[facei]]);

            sfCorrPtr[facei] =
               (CfPtr[facei] - CPtr[neiPtr[facei]]) & gradVfPtr[neiPtr[facei]];
        }

        scalar sfCorrs = magSqr(sfCorrPtr[facei]);
        scalar maxCorrs = sfCorrPtr[facei] & maxCorr;

        if (sfCorrs > 0)
        {
            if (maxCorrs < 0)
            {
                sfCorrPtr[facei] = Zero;
            }
            else if (sfCorrs > maxCorrs)
            {
                sfCorrPtr[facei] *= maxCorrs/(sfCorrs + VSMALL);
            }
        }
    };
    exec.parallelFor(Lambda, faceFlux.size());

    typename GeometricField<Type, fvsPatchField, surfaceMesh>::
        Boundary& bSfCorr = sfCorr.boundaryFieldRef();

    forAll(bSfCorr, patchi)
    {
        fvsPatchField<Type>& pSfCorr = bSfCorr[patchi];

        if (pSfCorr.coupled())
        {
            const labelUList& pOwner =
                mesh.boundary()[patchi].faceCells();

            const vectorField& pCf = Cf.boundaryField()[patchi];
            const scalarField& pW = w.boundaryField()[patchi];

            const scalarField& pFaceFlux = faceFlux.boundaryField()[patchi];

            const Field<typename outerProduct<vector, Type>::type> pGradVfNei
            (
                gradVf.boundaryField()[patchi].patchNeighbourField()
            );

            const Field<Type> pVfNei
            (
                vf.boundaryField()[patchi].patchNeighbourField()
            );

            // Build the d-vectors
            vectorField pd(Cf.boundaryField()[patchi].patch().delta());

            auto pSfCorrPtr = pSfCorr.begin();
            const auto pOwnerPtr = pOwner.cbegin();
            const auto pCfPtr = pCf.cbegin();
	    const auto pWPtr = pW.cbegin();
	    const auto pVfNeiPtr = pVfNei.cbegin();
	    const auto pdPtr = pd.cbegin();
	    const auto pGradVfNeiPtr = pGradVfNei.cbegin();
            const auto pFaceFluxPtr = pFaceFlux.cbegin();

	    auto LambdaPatch = [=](label facei)
            {
                label own = pOwnerPtr[facei];

                vector maxCorr;

                if (pFaceFluxPtr[facei] > 0)
                {
                    pSfCorrPtr[facei] = (pCfPtr[facei] - CPtr[own]) & gradVfPtr[own];

                    maxCorr = (1.0 - pWPtr[facei])*(pVfNeiPtr[facei] - vfPtr[own]);
                }
                else
                {
                    pSfCorrPtr[facei] =
                        (pCfPtr[facei] - pdPtr[facei] - CPtr[own]) & pGradVfNeiPtr[facei];

                    maxCorr = pWPtr[facei]*(vfPtr[own] - pVfNeiPtr[facei]);
                }

                scalar pSfCorrs = magSqr(pSfCorrPtr[facei]);
                scalar maxCorrs = pSfCorrPtr[facei] & maxCorr;

                if (pSfCorrs > 0)
                {
                    if (maxCorrs < 0)
                    {
                        pSfCorrPtr[facei] = Zero;
                    }
                    else if (pSfCorrs > maxCorrs)
                    {
                        pSfCorrPtr[facei] *= maxCorrs/(pSfCorrs + VSMALL);
                    }
                }
            };
            exec.parallelFor(LambdaPatch, pOwner.size());
        }
    }

    return tsfCorr;
}


namespace Foam
{
    makelimitedSurfaceInterpolationTypeScheme(linearUpwindV, vector)
}


// ************************************************************************* //
