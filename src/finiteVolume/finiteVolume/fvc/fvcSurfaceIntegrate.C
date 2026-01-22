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

#include "fvcSurfaceIntegrate.H"
#include "fvMesh.H"
#include "extrapolatedCalculatedFvPatchFields.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace fvc
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
void surfaceIntegrate
(
    Field<Type>& ivf,
    const GeometricField<Type, fvsPatchField, surfaceMesh>& ssf
)
{
    const fvMesh& mesh = ssf.mesh();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    const Field<Type>& issf = ssf;

    foamExecutor exec;
    auto ivfp = ivf.begin();
    const auto issfp = issf.cbegin();
    const auto op = owner.cbegin();
    const auto np = neighbour.cbegin();
    auto Lambda = [=](label facei)
    {
        foamAtomic::AtomicAdd(ivfp[op[facei]], issfp[facei]);
        foamAtomic::AtomicAdd(ivfp[np[facei]],-issfp[facei]);
    };
    exec.parallelFor(Lambda, owner.size());

    forAll(mesh.boundary(), patchi)
    {
        const labelUList& pFaceCells =
            mesh.boundary()[patchi].faceCells();

        const fvsPatchField<Type>& pssf = ssf.boundaryField()[patchi];

        const auto pFaceCellsp = pFaceCells.cbegin();
        const auto pssfp = pssf.cbegin();
        auto PatchLambda = [=](label facei)
        {
            foamAtomic::AtomicAdd(ivfp[pFaceCellsp[facei]], pssfp[facei]);
        };
        exec.parallelFor(PatchLambda, mesh.boundary()[patchi].size());
    };

    ivf /= mesh.Vsc();
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
surfaceIntegrate
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& ssf
)
{
    const fvMesh& mesh = ssf.mesh();

    tmp<GeometricField<Type, fvPatchField, volMesh>> tvf
    (
        new GeometricField<Type, fvPatchField, volMesh>
        (
            IOobject
            (
                "surfaceIntegrate("+ssf.name()+')',
                ssf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensioned<Type>(ssf.dimensions()/dimVol, Zero),
            fvPatchFieldBase::extrapolatedCalculatedType()
        )
    );
    GeometricField<Type, fvPatchField, volMesh>& vf = tvf.ref();

    surfaceIntegrate(vf.primitiveFieldRef(), ssf);
    vf.correctBoundaryConditions();

    return tvf;
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
surfaceIntegrate
(
    const tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>& tssf
)
{
    tmp<GeometricField<Type, fvPatchField, volMesh>> tvf
    (
        fvc::surfaceIntegrate(tssf())
    );
    tssf.clear();
    return tvf;
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>>
surfaceSum
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& ssf
)
{
    const fvMesh& mesh = ssf.mesh();

    tmp<GeometricField<Type, fvPatchField, volMesh>> tvf
    (
        new GeometricField<Type, fvPatchField, volMesh>
        (
            IOobject
            (
                "surfaceSum("+ssf.name()+')',
                ssf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensioned<Type>(ssf.dimensions(), Zero),
            fvPatchFieldBase::extrapolatedCalculatedType()
        )
    );
    GeometricField<Type, fvPatchField, volMesh>& vf = tvf.ref();

    const labelUList& owner = mesh.owner();
    const labelUList& neighbour = mesh.neighbour();

    foamExecutor exec;
    auto vfp = vf.begin();
    const auto op = owner.cbegin();
    const auto np = neighbour.cbegin();
    const auto ssfp = ssf.cbegin();
    auto Lambda = [=](label facei)
    {
        foamAtomic::AtomicAdd(vfp[op[facei]], ssfp[facei]);
        foamAtomic::AtomicAdd(vfp[np[facei]], ssfp[facei]);
    };
    exec.parallelFor(Lambda, owner.size());

    forAll(mesh.boundary(), patchi)
    {
        const labelUList& pFaceCells =
            mesh.boundary()[patchi].faceCells();

        const fvsPatchField<Type>& pssf = ssf.boundaryField()[patchi];

        const auto pssfp = pssf.cbegin();
        const auto pFaceCellsp = pFaceCells.cbegin();
        auto patchLambda = [=](label facei)
        {
           foamAtomic::AtomicAdd(vfp[pFaceCellsp[facei]], pssfp[facei]);
        };
        exec.parallelFor(patchLambda, mesh.boundary()[patchi].size());
    }

    vf.correctBoundaryConditions();

    return tvf;
}


template<class Type>
tmp<GeometricField<Type, fvPatchField, volMesh>> surfaceSum
(
    const tmp<GeometricField<Type, fvsPatchField, surfaceMesh>>& tssf
)
{
    tmp<GeometricField<Type, fvPatchField, volMesh>> tvf = surfaceSum(tssf());
    tssf.clear();
    return tvf;
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fvc

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// ************************************************************************* //
