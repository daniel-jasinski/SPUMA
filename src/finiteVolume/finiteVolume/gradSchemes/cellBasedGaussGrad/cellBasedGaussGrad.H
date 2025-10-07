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

Class
    Foam::fv::cellBasedGaussGrad

Group
    grpFvGradSchemes

Description
    Cell-based second-order gradient scheme using face-interpolation
    and Gauss' theorem with cell-centric loop structure.

SourceFiles
    cellBasedGaussGrad.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_cellBasedGaussGrad_H
#define Foam_cellBasedGaussGrad_H

#include "gradScheme.H"
#include "surfaceInterpolationScheme.H"
#include "linear.H"

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
                 Class cellBasedGaussGrad Declaration
\*---------------------------------------------------------------------------*/

template<class Type>
class cellBasedGaussGrad
:
    public fv::gradScheme<Type>
{
    //- Interpolation scheme
    tmp<surfaceInterpolationScheme<Type>> tinterpScheme_;

    // No copy
    cellBasedGaussGrad(const cellBasedGaussGrad&) = delete;
    void operator=(const cellBasedGaussGrad&) = delete;

public:

    // Selectable name in fvSchemes/gradSchemes
    TypeName("cellBasedGauss");

    // Constructors
    explicit cellBasedGaussGrad(const fvMesh& mesh)
    :
        gradScheme<Type>(mesh),
        tinterpScheme_(new linear<Type>(mesh))
    {}

    cellBasedGaussGrad(const fvMesh& mesh, Istream& is)
    :
        gradScheme<Type>(mesh),
        tinterpScheme_(nullptr)
    {
        if (is.eof())
        {
            tinterpScheme_.reset(new linear<Type>(mesh));
        }
        else
        {
            tinterpScheme_.reset(surfaceInterpolationScheme<Type>::New(mesh, is));
        }
    }

    // Member Functions

    //- Gradient from a surface field (already interpolated)
    static tmp
    <
        GeometricField
        <
            typename outerProduct<vector, Type>::type,
            fvPatchField, volMesh
        >
    > gradf
    (
        const GeometricField<Type, fvsPatchField, surfaceMesh>& ssf,
        const word& name
    );

    //- Gradient from a volume field (does interpolation, for caching)
    virtual tmp
    <
        GeometricField
        <
            typename outerProduct<vector, Type>::type,
            fvPatchField, volMesh
        >
    > calcGrad
    (
        const GeometricField<Type, fvPatchField, volMesh>& vsf,
        const word& name
    ) const;

    //- Same boundary correction as Gauss
    static void correctBoundaryConditions
    (
        const GeometricField<Type, fvPatchField, volMesh>& vsf,
        GeometricField
        <
            typename outerProduct<vector, Type>::type,
            fvPatchField, volMesh
        >& gGrad
    );
};

} // namespace fv
} // namespace Foam

#ifdef NoRepository
    #include "cellBasedGaussGrad.C"
#endif

#endif
