/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
    Copyright (C) 2025 CINECA
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
    Foam::ChebyshevSmoother

Group
    grpLduMatrixSmoothers

Description
    A lduMatrix::smoother implementing the Chebyshev smoother, a preconditioner 
    with a Chebyshev polynomial for symmetric positive definite matrices. 
    This preconditioner is based on an iteration of an inner preconditioner (Jacobi) 
    with coefficients that are adapted to optimally cover an eigenvalue range 
    between the largest eigenvalue λmax down to a given lower eigenvalue λmin.

SourceFiles
    ChebyshevSmoother.C

\*---------------------------------------------------------------------------*/

#ifndef ChebyshevSmoother_H
#define ChebyshevSmoother_H

#include "lduMatrix.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                           Class ChebyshevSmoother Declaration
\*---------------------------------------------------------------------------*/

class ChebyshevSmoother
:
    public lduMatrix::smoother
{
private:

    //- Chebyshev polynomial degree
    label pDegree_;

    //- Method to estimate the maximum eigenValue
    // 0: Power method
    // 1: Estimate based on the Gershgorin theorem
    // 2: User-defined value
    label lambdaMode_;

    //- (User-defined) maximum eigenValue
    scalar lambdaMax_;

    //- (User-defined) minimum eigenValue
    scalar lambdaMin_;

    //- Log level verbosity
    label log_;

    //- Read the control parameters from the controlDict_
    void readControls();

public:

    //- Runtime type information
    TypeName("Chebyshev");


    // Constructors

        //- Construct from components
        ChebyshevSmoother
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
