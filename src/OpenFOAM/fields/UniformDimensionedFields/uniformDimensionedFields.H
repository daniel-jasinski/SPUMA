/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2014 OpenFOAM Foundation
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

InClass
    Foam::UniformDimensionedField

Description
    Various UniformDimensionedField types

SourceFiles
    uniformDimensionedFields.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_uniformDimensionedFields_H
#define Foam_uniformDimensionedFields_H

#include "UniformDimensionedField.H"
#include "uniformDimensionedFieldsFwd.H"

#ifdef _WIN32
declareStaticTypeName(Foam::UniformDimensionedField<Foam::label>);
declareStaticTypeName(Foam::UniformDimensionedField<Foam::scalar>);
declareStaticTypeName(Foam::UniformDimensionedField<Foam::vector>);
declareStaticTypeName(Foam::UniformDimensionedField<Foam::sphericalTensor>);
declareStaticTypeName(Foam::UniformDimensionedField<Foam::symmTensor>);
declareStaticTypeName(Foam::UniformDimensionedField<Foam::tensor>);
#endif

#endif

// ************************************************************************* //
