/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

#include "linearUpwind.H"
#include "fvMesh.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp<Foam::GeometricField<Type, Foam::fvsPatchField, Foam::surfaceMesh>>
Foam::linearUpwind<Type>::correction
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
                "linearUpwind::correction(" + vf.name() + ')',
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            ),
            mesh,
            dimensioned<Type>(vf.name(), vf.dimensions(), Zero)
        )
    );

    GeometricField<Type, fvsPatchField, surfaceMesh>& sfCorr = tsfCorr.ref();

    const surfaceScalarField& faceFlux = this->faceFlux_;

    const labelList& owner = mesh.owner();
    const labelList& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    tmp<fv::gradScheme<scalar>> gradScheme_
    (
        fv::gradScheme<scalar>::New
        (
            mesh,
            mesh.gradScheme(gradSchemeName_)
        )
    );

    foamExecutor exec;
    const auto faceFluxp = faceFlux.cbegin();
    const auto ownerp = owner.cbegin();
    const auto neighbourp = neighbour.cbegin();
    const auto Cfp = Cf.cbegin();
    const auto Cp = C.cbegin();
          auto sfCorrp = sfCorr.begin();

    for (direction cmpt = 0; cmpt < pTraits<Type>::nComponents; cmpt++)
    {
        tmp<volVectorField> tgradVf =
            gradScheme_().grad(vf.component(cmpt), gradSchemeName_);

        const volVectorField& gradVf = tgradVf();
        const auto gradVfp = gradVf.cbegin();

        auto Lambda = [=](label facei){
            const label celli =
                (faceFluxp[facei] > 0) ? ownerp[facei] : neighbourp[facei];

            setComponent(sfCorrp[facei], cmpt) =
                (Cfp[facei] - Cp[celli]) & gradVfp[celli];
        };
        exec.parallelFor(Lambda,faceFlux.size());

        // forAll(faceFlux, facei)
        // {
        //     const label celli =
        //         (faceFlux[facei] > 0) ? owner[facei] : neighbour[facei];

        //     setComponent(sfCorr[facei], cmpt) =
        //         (Cf[facei] - C[celli]) & gradVf[celli];
        // }

        typename GeometricField<Type, fvsPatchField, surfaceMesh>::
            Boundary& bSfCorr = sfCorr.boundaryFieldRef();

        forAll(bSfCorr, patchi)
        {
            fvsPatchField<Type>& pSfCorr = bSfCorr[patchi];
                 auto pSfCorrp = pSfCorr.begin();

            if (pSfCorr.coupled())
            {
                const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
                const vectorField& pCf = Cf.boundaryField()[patchi];
                const scalarField& pFaceFlux = faceFlux.boundaryField()[patchi];

                const vectorField pGradVfNei
                (
                    gradVf.boundaryField()[patchi].patchNeighbourField()
                );

                // Build the d-vectors
                const vectorField pd
                (
                    Cf.boundaryField()[patchi].patch().delta()
                );

                //define executor pointers
                const auto pFaceFluxp = pFaceFlux.cbegin();
                const auto pOwnerp = pOwner.cbegin();
                const auto pCfp = pCf.cbegin();
                const auto pdp = pd.cbegin();
                const auto pGradVfNeip = pGradVfNei.cbegin();

                auto Lambda = [=](label facei){
                    label own = pOwnerp[facei];

                    if (pFaceFluxp[facei] > 0)
                    {
                        setComponent(pSfCorrp[facei], cmpt) =
                            (pCfp[facei] - Cp[own])
                          & gradVfp[own];
                    }
                    else
                    {
                        setComponent(pSfCorrp[facei], cmpt) =
                            (pCfp[facei] - pdp[facei] - Cp[own])
                          & pGradVfNeip[facei];
                    }
                };
                exec.parallelFor(Lambda,pOwner.size());
                // forAll(pOwner, facei)
                // {
                //     label own = pOwner[facei];

                //     if (pFaceFlux[facei] > 0)
                //     {
                //         setComponent(pSfCorr[facei], cmpt) =
                //             (pCf[facei] - C[own])
                //           & gradVf[own];
                //     }
                //     else
                //     {
                //         setComponent(pSfCorr[facei], cmpt) =
                //             (pCf[facei] - pd[facei] - C[own])
                //           & pGradVfNei[facei];
                //     }
                // }
            }
        }
    }

    return tsfCorr;
}


template<>
Foam::tmp<Foam::surfaceVectorField>
Foam::linearUpwind<Foam::vector>::correction
(
    const volVectorField& vf
) const
{
    const fvMesh& mesh = this->mesh();

    tmp<surfaceVectorField> tsfCorr
    (
        new surfaceVectorField
        (
            IOobject
            (
                "linearUpwind::correction(" + vf.name() + ')',
                mesh.time().timeName(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE,
                IOobject::NO_REGISTER
            ),
            mesh,
            dimensioned<vector>(vf.name(), vf.dimensions(), Zero)
        )
    );

    surfaceVectorField& sfCorr = tsfCorr.ref();

    const surfaceScalarField& faceFlux = this->faceFlux_;

    const labelList& owner = mesh.owner();
    const labelList& neighbour = mesh.neighbour();

    const volVectorField& C = mesh.C();
    const surfaceVectorField& Cf = mesh.Cf();

    tmp<fv::gradScheme<vector>> gradScheme_
    (
        fv::gradScheme<vector>::New
        (
            mesh,
            mesh.gradScheme(gradSchemeName_)
        )
    );

    tmp<volTensorField> tgradVf = gradScheme_().grad(vf, gradSchemeName_);
    const volTensorField& gradVf = tgradVf();

    foamExecutor exec;
    const auto gradVfp = gradVf.cbegin();
    const auto faceFluxp = faceFlux.cbegin();
    const auto ownerp = owner.cbegin();
    const auto neighbourp = neighbour.cbegin();
    const auto Cfp = Cf.cbegin();
    const auto Cp = C.cbegin();
          auto sfCorrp = sfCorr.begin();

    auto Lambda = [=](label facei){
        const label celli =
            (faceFluxp[facei] > 0) ? ownerp[facei] : neighbourp[facei];
        sfCorrp[facei] = (Cfp[facei] - Cp[celli]) & gradVfp[celli];
    };
    exec.parallelFor(Lambda,faceFlux.size());

    // forAll(faceFlux, facei)
    // {
    //     const label celli =
    //         (faceFlux[facei] > 0) ? owner[facei] : neighbour[facei];
    //     sfCorr[facei] = (Cf[facei] - C[celli]) & gradVf[celli];
    // }


    typename surfaceVectorField::Boundary& bSfCorr = sfCorr.boundaryFieldRef();

    forAll(bSfCorr, patchi)
    {
        fvsPatchVectorField& pSfCorr = bSfCorr[patchi];

        if (pSfCorr.coupled())
        {
            const labelUList& pOwner = mesh.boundary()[patchi].faceCells();
            const vectorField& pCf = Cf.boundaryField()[patchi];
            const scalarField& pFaceFlux = faceFlux.boundaryField()[patchi];

            const tensorField pGradVfNei
            (
                gradVf.boundaryField()[patchi].patchNeighbourField()
            );

            // Build the d-vectors
            vectorField pd(Cf.boundaryField()[patchi].patch().delta());

            //define executor pointers
            const auto pFaceFluxp = pFaceFlux.cbegin();
            const auto pOwnerp = pOwner.cbegin();
            const auto pCfp = pCf.cbegin();
            const auto pdp = pd.cbegin();
            const auto pGradVfNeip = pGradVfNei.cbegin();
                  auto pSfCorrp = pSfCorr.begin();

            auto Lambda = [=](label facei){
                label own = pOwnerp[facei];

                if (pFaceFluxp[facei] > 0)
                {
                    pSfCorrp[facei] = (pCfp[facei] - Cp[own]) & gradVfp[own];
                }
                else
                {
                    pSfCorrp[facei] =
                        (pCfp[facei] - pdp[facei] - Cp[own]) & pGradVfNeip[facei];
                }
            };
            exec.parallelFor(Lambda,pOwner.size());
            // forAll(pOwner, facei)
            // {
            //     label own = pOwner[facei];

            //     if (pFaceFlux[facei] > 0)
            //     {
            //         pSfCorr[facei] = (pCf[facei] - C[own]) & gradVf[own];
            //     }
            //     else
            //     {
            //         pSfCorr[facei] =
            //             (pCf[facei] - pd[facei] - C[own]) & pGradVfNei[facei];
            //     }
            // }
        }
    }

    return tsfCorr;
}


namespace Foam
{
    makelimitedSurfaceInterpolationScheme(linearUpwind)
}

// ************************************************************************* //
