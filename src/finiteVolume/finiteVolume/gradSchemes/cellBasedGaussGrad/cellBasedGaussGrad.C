/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2021 OpenCFD Ltd.
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

#include "cellBasedGaussGrad.H"
#include "extrapolatedCalculatedFvPatchField.H"
#include "lduAddressing.H" 
#include "fvMeshCsrAddressing.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type>
Foam::tmp
<
    Foam::GeometricField
    <
        typename Foam::outerProduct<Foam::vector, Type>::type,
        Foam::fvPatchField,
        Foam::volMesh
    >
>
Foam::fv::cellBasedGaussGrad<Type>::gradf
(
    const GeometricField<Type, fvsPatchField, surfaceMesh>& ssf,
    const word& name
)
{
    typedef typename outerProduct<vector, Type>::type GradType;
    typedef GeometricField<GradType, fvPatchField, volMesh> GradFieldType;

    const fvMesh& mesh = ssf.mesh();

    tmp<GradFieldType> tgGrad
    (
        new GradFieldType
        (
            IOobject
            (
                name,
                ssf.instance(),
                mesh,
                IOobject::NO_READ,
                IOobject::NO_WRITE
            ),
            mesh,
            dimensioned<GradType>(ssf.dimensions()/dimLength),
            fvPatchFieldBase::extrapolatedCalculatedType()
        )
    );

    GradFieldType& gGrad = tgGrad.ref();

    Field<GradType>& igGrad = gGrad.internalFieldRef();

    // The oriented area vectors for internal faces (owner -> neighbour) 
    const auto& Sfi   = mesh.Sf().internalField();

    // The interpolated surface scalar field at internal faces 
    const auto& ssfi  = ssf.internalField();

    const label nIntFaces = mesh.nInternalFaces();

    // Use CSR adapter from mesh, built on demand (once per mesh)
    const auto& csrAddr = mesh.csrAddr();
    const scalarList& V = mesh.V();

    // Raw pointers
    const label* const __restrict__ offsPtr = csrAddr.offsets().cbegin();
    const label* const __restrict__ idxPtr  = csrAddr.indices().cbegin();
    const label* const __restrict__ sgnPtr  = csrAddr.signs().cbegin();
    const scalar* const __restrict__ VPtr = V.cbegin();

    GradType* __restrict__ igGradPtr = igGrad.begin();
    const vector* const __restrict__ SfiPtr = Sfi.cbegin();
    const Type* const __restrict__ ssfiPtr = ssfi.cbegin();

    auto gradLoopKernel = [=](label celli)
    {
        // accumulate in a register, write once 
        GradType acc = Zero;

        for (label i = offsPtr[celli]; i < offsPtr[celli + 1]; ++i)
        {
            const label facei = idxPtr[i];
            const GradType Sfphi = SfiPtr[facei] * ssfiPtr[facei];
            acc += Sfphi * static_cast<scalar>(sgnPtr[i]);
        }

        igGradPtr[celli] = acc / VPtr[celli];
    };

    foamExecutor exec;
    exec.parallelFor(gradLoopKernel, mesh.nCells());

    // ---- Finalize
    gGrad.correctBoundaryConditions();

    return tgGrad;
}


template<class Type>
Foam::tmp
<
    Foam::GeometricField
    <
        typename Foam::outerProduct<Foam::vector, Type>::type,
        Foam::fvPatchField,
        Foam::volMesh
    >
>
Foam::fv::cellBasedGaussGrad<Type>::calcGrad
(
    const GeometricField<Type, fvPatchField, volMesh>& vsf,
    const word& name
) const
{
    typedef typename outerProduct<vector, Type>::type GradType;
    typedef GeometricField<GradType, fvPatchField, volMesh> GradFieldType;

    tmp<GradFieldType> tgGrad
    (
        gradf(tinterpScheme_().interpolate(vsf), name)
    );
    GradFieldType& gGrad = tgGrad.ref();

    correctBoundaryConditions(vsf, gGrad);
    return tgGrad;
}


template<class Type>
void Foam::fv::cellBasedGaussGrad<Type>::correctBoundaryConditions
(
    const GeometricField<Type, fvPatchField, volMesh>& vsf,
    GeometricField
    <
        typename outerProduct<vector, Type>::type,
        fvPatchField, volMesh
    >& gGrad
)
{
    auto& gGradbf = gGrad.boundaryFieldRef();

    forAll(vsf.boundaryField(), patchi)
    {
        if (!vsf.boundaryField()[patchi].coupled())
        {
            const vectorField n
            (
                vsf.mesh().Sf().boundaryField()[patchi]
              / vsf.mesh().magSf().boundaryField()[patchi]
            );

            gGradbf[patchi] += n *
            (
                vsf.boundaryField()[patchi].snGrad()
              - (n & gGradbf[patchi])
            );
        }
    }
}

