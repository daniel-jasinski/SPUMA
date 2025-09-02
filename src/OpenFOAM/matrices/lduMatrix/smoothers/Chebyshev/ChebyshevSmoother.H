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
    The implementation take form a 3 therm recurrence as stated in 
    \verbatim
        https://www.dealii.org/current/doxygen/deal.II/classPreconditionChebyshev.html
    \verbatim
    This preconditioner is based on an iteration of an inner preconditioner (Jacobi or L1-jacobi)
    with coefficients that are adapted to optimally cover an eigenvalue range 
    between the largest eigenvalue λmax down to a given lower eigenvalue λmin 
    or given by the optimal values for GAMG error projection using V-cycle with N pre and post
    sweeps of polynomal smoother of 1-th kind Chebyshev of fixed degree k described in:
    \verbatim
        D’Ambra, P., Durastante, F., Filippone, S. et al. 
        Optimal polynomial smoothers for parallel AMG. Numer Algor (2025). 
        https://doi.org/10.1007/s11075-025-02117-6
    \endverbatim

Usage:
    \table
        Property              | Description               | Required    | Default value
        pDegree               | polynomial degree         | no          | 1
        rescaling             | polynomial scaling bounds | yes         | userDefined
        spRadius              | spectral Radius estimate  | no          | Ghershgorin theorem
        preconditionerName    | preconditioner Type       | yes         |
        lambdaMin             | lower rescaling value     | optional    | 1./8
        log                   | log verbosity value       | optional    | 0
    \endtable


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

    //- Method used for rescaling
    // userDefined: User-defined value
    // optimalVCycle: optimal mode for V-cycle GAMG projection error
    word normalization_;

    //- (User-defined) maximum eigenValue
    const scalar lambdaMax_ = 1.0;

    //- (User-defined) minimum eigenValue
    scalar lambdaMin_;

    //- preconditioner name
    word subPreconditionerName_;
    
    //- ptr of preconditioner
    autoPtr<lduMatrix::preconditioner> preconditioner_;

    //- spectral radius estimator
    autoPtr<eigenValueSolver> spRadiusEstimator_;

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
