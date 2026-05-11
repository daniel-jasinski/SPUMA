/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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

Class
    Foam::TmulticolorGaussSeidelSmoother

Group
    grpLduMatrixSmoothers

Description
    A race-condition-free implementation (based on color graph partitioning)
    of Gauss-Seidel designed for multithreading.

Usage
    Example usage:
    \verbatim
    multicolorGaussSeidelSmoother
    {
        graphColoring      randomized;
        deterministic      true;
        shrinkingFactor    1.0;
    }
    \endverbatim

SourceFiles
    TmulticolorGaussSeidelSmoother.H
    TmulticolorGaussSeidelSmoother.C

\*---------------------------------------------------------------------------*/

#ifndef TmulticolorGaussSeidelSmoother_H
#define TmulticolorGaussSeidelSmoother_H

#include "LduMatrix.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class TmulticolorGaussSeidel Smoother Declaration
\*---------------------------------------------------------------------------*/

template<class Type, class DType, class LUType>
class TmulticolorGaussSeidelSmoother
:
    public LduMatrix<Type, DType, LUType>::smoother
{
private:

    // Private data
    
        //- The inverse (reciprocal for scalars) diagonal
        Field<DType> rD_;


public:

    //- Runtime type information
    TypeName("multicolorGaussSeidel");


    // Constructors

        //- Construct from components
        TmulticolorGaussSeidelSmoother
        (
            const word& fieldName,
            const LduMatrix<Type, DType, LUType>& matrix
        );


    // Member Functions

        //- Smooth for the given number of sweeps
        static void smooth
        (
            const word& fieldName,
            const dictionary& controlDict,
            Field<Type>& psi,
            const LduMatrix<Type, DType, LUType>& matrix,
            const Field<DType>& rD,
            const label nSweeps
        );

        //- Smooth the solution for a given number of sweeps
        virtual void smooth
        (
            Field<Type>& psi,
            const label nSweeps
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "TmulticolorGaussSeidelSmoother.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
