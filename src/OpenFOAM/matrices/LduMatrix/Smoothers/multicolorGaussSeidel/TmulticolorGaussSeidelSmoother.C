/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2015 OpenFOAM Foundation
    Copyright (C) 2017-2019 OpenCFD Ltd.
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

#include "TmulticolorGaussSeidelSmoother.H"
#include "PrecisionAdaptor.H"


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

template<class Type, class DType, class LUType>
Foam::TmulticolorGaussSeidelSmoother<Type, DType, LUType>::TmulticolorGaussSeidelSmoother
(
    const word& fieldName,
    const LduMatrix<Type, DType, LUType>& matrix
)
:
    LduMatrix<Type, DType, LUType>::smoother
    (
        fieldName,
        matrix
    ),
    rD_(matrix.diag().size())
{
    const label nCells = matrix.diag().size();
    const DType* const __restrict__ diagPtr = matrix.diag().begin();
    DType* __restrict__ rDPtr = rD_.begin();
    Type localOne(pTraits<Type>::one);

    foamExecutor exec;

    auto LambdaInvDiag = [=](label celli)
    {
        if constexpr(std::is_same<DType, Type>::value)
        {
            rDPtr[celli] = cmptDivide(localOne, diagPtr[celli]);
        }
        else
        {
            rDPtr[celli] = inv(diagPtr[celli]);
        }
    };
    exec.parallelFor(LambdaInvDiag, nCells);
}

// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

template<class Type, class DType, class LUType>
void Foam::TmulticolorGaussSeidelSmoother<Type, DType, LUType>::smooth
(
    const word& fieldName_,
    const dictionary& controlDict,
    Field<Type>& psi,
    const LduMatrix<Type, DType, LUType>& matrix_,
    const Field<DType>& rD_,
    const label nSweeps
)
{
    Type* __restrict__ psiPtr = psi.begin();
    const Type* const __restrict__ bPtr = matrix_.source().cbegin();

    const label nCells = psi.size();
    const label nInternalFaces = matrix_.upper().size();

    Field<Type> bPrime(nCells);
    Type* __restrict__ bPrimePtr = bPrime.begin();

    const DType* const __restrict__ diagPtr = matrix_.diag().cbegin();

    const LUType* const __restrict__ upperPtr =
        matrix_.upper().begin();
    const LUType* const __restrict__ lowerPtr =
        matrix_.lower().begin();
    const LUType* const __restrict__ lowercsrPtr = 
        matrix_.lowerCSR().begin();

    const lduAddressing& addr = matrix_.lduAddr();

    const label* const __restrict__ uPtr =
        addr.upperAddr().begin();
    const label* const __restrict__ lPtr =
        addr.lowerAddr().begin();

    const label* const __restrict__ oStartPtr =
        addr.ownerStartAddr().begin();
    const label* const __restrict__ loStartPtr =
        addr.losortStartAddr().begin();
    const label* const __restrict__ lcsrPtr =
        addr.lowerCSRAddr().begin();

    const DType* const __restrict__ rDPtr = rD_.cbegin();

    dictionary smootherDict;
    if (controlDict.found(TmulticolorGaussSeidelSmoother<Type, DType, LUType>::typeName))
    {
        smootherDict = controlDict.subDict
        (
            TmulticolorGaussSeidelSmoother<Type, DType, LUType>::typeName
        );
    }

    const List<DynamicList<label>>& partitions = addr.partitions(smootherDict);

    foamExecutor exec;

    // Parallel boundary initialisation.  The parallel boundary is treated
    // as an effective jacobi interface in the boundary.
    // Note: there is a change of sign in the coupled
    // interface update.  The reason for this is that the
    // internal coefficients are all located at the l.h.s. of
    // the matrix whereas the "implicit" coefficients on the
    // coupled boundaries are all created as if the
    // coefficient contribution is of a source-kind (i.e. they
    // have a sign as if they are on the r.h.s. of the matrix.
    // To compensate for this, it is necessary to turn the
    // sign of the contribution.

    for (label sweep=0; sweep<nSweeps; sweep++)
    {
        bPrime = matrix_.source();

        const label startRequest = UPstream::nRequests();

        matrix_.initMatrixInterfaces
        (
            false,
            matrix_.interfacesUpper(),
            psi,
            bPrime
        );

        matrix_.updateMatrixInterfaces
        (
            false,
            matrix_.interfacesUpper(),
            psi,
            bPrime,
            startRequest
        );

        for (const DynamicList<label>& partition : partitions)
        {
            const label* const __restrict__ partitionPtr = partition.cbegin();

            auto LambdaColor = [=](label i)
            {
                const label celli = partitionPtr[i];

                Type psii;

                // Get the accumulated neighbour side
                psii = bPrimePtr[celli];

                // Add lower contributions
                {
                    const label start = loStartPtr[celli];
                    const label end = loStartPtr[celli + 1];

                    for (label i=start; i<end; ++i)
                    {
                        const label nbrCell = lcsrPtr[i];
                        psii -= dot(lowercsrPtr[i], psiPtr[nbrCell]);
                    }
                }

                // Add upper contributions
                {
                    const label start = oStartPtr[celli]; 
                    const label end = oStartPtr[celli + 1];

                    for (label i=start; i<end; ++i)
                    {
                        const label nbrCell = uPtr[i];
                        psii -= dot(upperPtr[i], psiPtr[nbrCell]);
                    }
                }

                // Finish current psi for this cell
                if constexpr(std::is_same<DType, Type>::value)
                {
                    psii = cmptMultiply(rDPtr[celli], psii);
                }
                else
                {
                    psii = dot(rDPtr[celli], psii);
                }

                psiPtr[celli] = psii;
            };
            exec.parallelFor(LambdaColor, partition.size());
        }
    }
}


template<class Type, class DType, class LUType>
void Foam::TmulticolorGaussSeidelSmoother<Type, DType, LUType>::smooth
(
    Field<Type>& psi,
    const label nSweeps
) const
{
    smooth(this->fieldName_, this->controlDict_, psi, this->matrix_, rD_, nSweeps);
}

// ************************************************************************* //
