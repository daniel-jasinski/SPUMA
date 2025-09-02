/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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
    Foam::RichardsonSmoother

Group
    grpLduMatrixSmoothers

Description
    Modified (preconditioned) Richardson iteration.
    Also known as damped Jacobi method.

SourceFiles
    Richardson.C

\*---------------------------------------------------------------------------*/

#ifndef RichardsonSmoother_H
#define RichardsonSmoother_H

#include "lduMatrix.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class RichardsonSmoother Declaration
\*---------------------------------------------------------------------------*/

class RichardsonSmoother
:
    public lduMatrix::smoother
{
private:

    //- damping parameter
    scalar omega_;

    //- preconditioner name
    word subPreconditionerName_;
    
    //- ptr of preconditioner
    autoPtr<lduMatrix::preconditioner> preconditioner_;

    //- Read the control parameters from the controlDict_
    void readControls();

public:

    //- Runtime type information
    TypeName("Richardson");


    // Constructors

        //- Construct from components
        RichardsonSmoother
        (
            const word& fieldName,
            const lduMatrix& matrix,
            const FieldField<Field, scalar>& interfaceBouCoeffs,
            const FieldField<Field, scalar>& interfaceIntCoeffs,
            const lduInterfaceFieldPtrsList& interfaces,
            const dictionary& solverControls
        );


    // Member Functions

        //- Smooth for the given number of sweeps
        void smooth_
        (
            const word& fieldName,
            solveScalarField& psi,
            const lduMatrix& matrix,
            const solveScalarField& source,
            const FieldField<Field, scalar>& interfaceBouCoeffs,
            const lduInterfaceFieldPtrsList& interfaces,
            const direction cmpt,
            const label nSweeps
        ) const;


        //- Smooth the solution for a given number of sweeps
        virtual void smooth
        (
            solveScalarField& psi,
            const scalarField& Source,
            const direction cmpt,
            const label nSweeps
        ) const;

        //- Smooth the solution for a given number of sweeps
        virtual void scalarSmooth
        (
            solveScalarField& psi,
            const solveScalarField& Source,
            const direction cmpt,
            const label nSweeps
        ) const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
