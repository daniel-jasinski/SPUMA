/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2025 CINECA.
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
    Foam::powerMethod

Group
    grpEigenValueSolvers

Description
    Computation of the maximum eigenvalue using the Power method.

SourceFiles
    powerMethod.C

\*---------------------------------------------------------------------------*/

#ifndef powerMethod_H
#define powerMethod_H

#include "lduMatrix.H"
#include "eigenValueSolver.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class powerMethod Declaration
\*---------------------------------------------------------------------------*/

class powerMethod
: 
    public eigenValueSolver
{

public:

    //- Runtime type information
    TypeName("powerMethod");


    // Constructors

        //- constructor

        powerMethod()
        : eigenValueSolver()
        {};

        powerMethod(Istream&)
        : eigenValueSolver()
        {};

    // Member Functions

        //- get maximum eigenvalue of a matrix
        virtual scalar maxEigenvalue
        (
            const lduMatrix& matrix_,
            const FieldField<Field, scalar>& interfaceBouCoeffs_,
            const lduInterfaceFieldPtrsList& interfaces_,
            const direction cmpt
        );
        
        //- get maximum eigenvalue of a matrix preconditioned
        //  with a diagonal preconditioner
        virtual scalar maxEigenvalue
        (
            const lduMatrix& matrix_,
            const lduMatrix::preconditioner& preconditioner_,
            const FieldField<Field, scalar>& interfaceBouCoeffs_,
            const lduInterfaceFieldPtrsList& interfaces_,
            const direction cmpt
        );
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
