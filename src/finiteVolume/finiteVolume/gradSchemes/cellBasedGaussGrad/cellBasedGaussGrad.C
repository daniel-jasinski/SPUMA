/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2018-2021 OpenCFD Ltd.
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
            dimensioned<GradType>(ssf.dimensions()/dimLength, Zero),
            fvPatchFieldBase::extrapolatedCalculatedType()
        )
    );

    GradFieldType& gGrad = tgGrad.ref();

    Field<GradType>& igGrad = gGrad.internalFieldRef();

    // The oriented area vectors for internal faces (owner -> neighbour) 
    const vectorField& SfInt   = mesh.Sf().internalField();

    // The interpolated surface scalar field at internal faces 
    const Field<Type>& ssfInt  = ssf.internalField();

    // Use CSR adapter from mesh, built on demand (once per mesh)
    const auto& csrAddr = mesh.csrAddr();
    const auto offsp = csrAddr.offsets().cbegin();     // nCells+1
    const auto idxp  = csrAddr.indices().cbegin();     // nnz
    const auto sgnp  = csrAddr.signs().cbegin();       // nnz, they are the +1 and -1

    // Raw pointers 
    auto igGradp = igGrad.begin();   // write
    auto Sfp  = SfInt.cbegin();   // read
    auto phip = ssfInt.cbegin();  // read

    auto Kernel = [=](label c)
    {
        // accumulate in a register, write once 
        GradType acc = igGradp[c]; 

        for (label k = offsp[c]; k < offsp[c+1]; ++k)
        {
            const label f = idxp[k];
            const GradType Sfphi = Sfp[f] * phip[f];

            acc += Sfphi * static_cast<scalar>(sgnp[k]);
        }

        igGradp[c] = acc;
    };

    foamExecutor exec;
    exec.parallelFor(Kernel, mesh.nCells());
  
    // Boundary contributions: owner-only, identical to Gauss
    forAll(mesh.boundary(), patchi)
    {
        const labelUList& pFaceCells = mesh.boundary()[patchi].faceCells();

        const vectorField& pSf = mesh.Sf().boundaryField()[patchi];
        const fvsPatchField<Type>& pssf = ssf.boundaryField()[patchi];

        const auto pSfp        = pSf.cbegin();
        const auto pssfp       = pssf.cbegin();
        const auto pFaceCellsp = pFaceCells.cbegin();
        auto igGradp           = igGrad.begin();

        auto LambdaFaces = [=](label iFace)
        {
            // atomic because different faces may hit the same cell concurrently
            foamAtomic::AtomicAdd(igGradp[pFaceCellsp[iFace]], pSfp[iFace]*pssfp[iFace]);
        };

        foamExecutor exec;
        exec.parallelFor(LambdaFaces, mesh.boundary()[patchi].size());
    }

    // ---- Finalize
    igGrad /= mesh.V();
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

