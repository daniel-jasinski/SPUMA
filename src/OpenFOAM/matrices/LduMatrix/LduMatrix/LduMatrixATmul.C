/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2017-2023 OpenCFD Ltd.
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

#include "LduMatrix.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

template<class Type, class DType, class LUType>
void Foam::LduMatrix<Type, DType, LUType>::Amul
(
    Field<Type>& Apsi,
    const tmp<Field<Type>>& tpsi
) const
{
    const auto& addr = lduAddr();

    Type* __restrict__ ApsiPtr = Apsi.begin();

    const Field<Type>& psi = tpsi();
    const Type* const __restrict__ psiPtr = psi.begin();

    const DType* const __restrict__ diagPtr = diag().begin();

    const label* const __restrict__ uPtr = lduAddr().upperAddr().begin();
    const label* const __restrict__ lPtr = lduAddr().lowerAddr().begin();

    const LUType* const __restrict__ upperPtr = upper().begin();
    const LUType* const __restrict__ lowerPtr = lower().begin();

    const label startRequest = UPstream::nRequests();

    foamExecutor exec;

    // Initialise the update of interfaced interfaces
    initMatrixInterfaces
    (
        true,
        interfacesUpper_,
        psi,
        Apsi
    );

    const label nCells = diag().size();
    
    if (hasLowerCSR())
    {
        // Use cell-based looping
        if (debugLevel() == 2) PoutInFunction<< "cell-based looping" << endl;

        const label* const __restrict__ oStartPtr =
            addr.ownerStartAddr().begin();
        const label* const __restrict__ loStartPtr =
            addr.losortStartAddr().begin();
        const label* const __restrict__ lcsrPtr =
                addr.lowerCSRAddr().begin();

        // Note: lowerCSR constructed from lower if available, upper otherwise
        //       so is handling symmetric()
        const scalar* const __restrict__ lowercsrPtr = lowerCSR().begin();

        auto LambdaAmul = [=](label cell)
        {
            auto& val = ApsiPtr[cell];

            if constexpr(std::is_same<DType, Type>::value)
            {
                val = cmptMultiply(diagPtr[cell], psiPtr[cell]);
            }
            else
            {
                val = dot(diagPtr[cell], psiPtr[cell]);
            }

            // Add lower contributions
            {
                const label start = loStartPtr[cell];
                const label end = loStartPtr[cell+1];

                for (label i = start; i < end; i++)
                {
                    const label nbrCell = lcsrPtr[i];
                    val += dot(lowercsrPtr[i], psiPtr[nbrCell]);
                }
            }
            // Add upper contributions
            {
                const label start = oStartPtr[cell];
                const label end = oStartPtr[cell+1];

                for (label i = start; i < end; i++)
                {
                    const label nbrCell = uPtr[i];
                    val += dot(upperPtr[i], psiPtr[nbrCell]);
                }
            }
        };
        exec.parallelFor(LambdaAmul, nCells);
    }
    else
    {
        auto LamdaDiag = [=](label cell)
        {
            if constexpr(std::is_same<DType, Type>::value)
            {
                ApsiPtr[cell] = cmptMultiply(diagPtr[cell], psiPtr[cell]);
            }
            else
            {
                ApsiPtr[cell] = dot(diagPtr[cell], psiPtr[cell]);
            }
        };
        exec.parallelFor(LamdaDiag, nCells);

        const label nFaces = upper().size();

        auto LambdaOffDiag = [=](label face)
        {
            foamAtomic::AtomicAdd(ApsiPtr[uPtr[face]], dot(lowerPtr[face], psiPtr[lPtr[face]]));
            foamAtomic::AtomicAdd(ApsiPtr[lPtr[face]], dot(upperPtr[face], psiPtr[uPtr[face]]));
        };
        exec.parallelFor(LambdaOffDiag, nFaces);
    }

    // Update interface interfaces
    updateMatrixInterfaces
    (
        true,
        interfacesUpper_,
        psi,
        Apsi,
        startRequest
    );

    tpsi.clear();
}


template<class Type, class DType, class LUType>
void Foam::LduMatrix<Type, DType, LUType>::Tmul
(
    Field<Type>& Tpsi,
    const tmp<Field<Type>>& tpsi
) const
{
    Type* __restrict__ TpsiPtr = Tpsi.begin();

    const Field<Type>& psi = tpsi();
    const Type* const __restrict__ psiPtr = psi.begin();

    const DType* const __restrict__ diagPtr = diag().begin();

    const label* const __restrict__ uPtr = lduAddr().upperAddr().begin();
    const label* const __restrict__ lPtr = lduAddr().lowerAddr().begin();

    const LUType* const __restrict__ lowerPtr = lower().begin();
    const LUType* const __restrict__ upperPtr = upper().begin();

    const label startRequest = UPstream::nRequests();

    foamExecutor exec;

    // Initialise the update of interfaced interfaces
    initMatrixInterfaces
    (
        true,
        interfacesLower_,
        psi,
        Tpsi
    );

    const label nCells = diag().size();
        
    auto LambdaDiag = [=](label cell)
    {
        if constexpr(std::is_same<DType, Type>::value)
        {
            TpsiPtr[cell] = cmptMultiply(diagPtr[cell], psiPtr[cell]);
        }
        else
        {
            TpsiPtr[cell] = dot(diagPtr[cell], psiPtr[cell]);
        }
    };
    exec.parallelFor(LambdaDiag, nCells);

    const label nFaces = upper().size();

    auto LambdaOffDiag = [=](label face)
    {
        foamAtomic::AtomicAdd(TpsiPtr[uPtr[face]], dot(upperPtr[face], psiPtr[lPtr[face]]));
        foamAtomic::AtomicAdd(TpsiPtr[lPtr[face]], dot(lowerPtr[face], psiPtr[uPtr[face]]));
    };
    exec.parallelFor(LambdaOffDiag, nFaces);

    // Update interface interfaces
    updateMatrixInterfaces
    (
        true,
        interfacesLower_,
        psi,
        Tpsi,
        startRequest
    );

    tpsi.clear();
}


template<class Type, class DType, class LUType>
void Foam::LduMatrix<Type, DType, LUType>::sumA
(
    Field<Type>& sumA
) const
{
    const auto& addr = lduAddr();

    Type* __restrict__ sumAPtr = sumA.begin();

    const DType* __restrict__ diagPtr = diag().begin();

    const label* __restrict__ uPtr = lduAddr().upperAddr().begin();
    const label* __restrict__ lPtr = lduAddr().lowerAddr().begin();

    const LUType* __restrict__ lowerPtr = lower().begin();
    const LUType* __restrict__ upperPtr = upper().begin();

    const label nCells = diag().size();
    const label nFaces = upper().size();

    foamExecutor exec;

    Type localOne(pTraits<Type>::one);
    
    if (hasLowerCSR())
    {
        // Use cell-based looping
        if (debugLevel() == 2) PoutInFunction<< "cell-based looping" << endl;

        const label* const __restrict__ oStartPtr =
            addr.ownerStartAddr().begin();
        const label* const __restrict__ loStartPtr =
            addr.losortStartAddr().begin();
        const label* const __restrict__ lcsrPtr =
                addr.lowerCSRAddr().begin();

        // Note: lowerCSR constructed from lower if available, upper otherwise
        //       so is handling symmetric()
        const scalar* const __restrict__ lowercsrPtr = lowerCSR().begin();

        auto LambdaAmul = [=](label cell)
        {
            auto& val = sumAPtr[cell];

            if constexpr(std::is_same<DType, Type>::value)
            {
                val = diagPtr[cell];
            }
            else
            {
                val = dot(diagPtr[cell], localOne);
            }

            // Add lower contributions
            {
                const label start = loStartPtr[cell];
                const label end = loStartPtr[cell+1];

                for (label i = start; i < end; i++)
                {
                    val += dot(lowercsrPtr[i], localOne);
                }
            }
            // Add upper contributions
            {
                const label start = oStartPtr[cell];
                const label end = oStartPtr[cell+1];

                for (label i = start; i < end; i++)
                {
                    val += dot(upperPtr[i], localOne);
                }
            }
        };
        exec.parallelFor(LambdaAmul, nCells);
    }
    else
    {
        auto LambdaDiag = [=](label cell)
        {
            if constexpr(std::is_same<DType, Type>::value)
            {
                sumAPtr[cell] = diagPtr[cell];
            }
            else
            {
                sumAPtr[cell] = dot(diagPtr[cell], localOne);
            }
        };
        exec.parallelFor(LambdaDiag, nCells);

        auto LambdaOffDiag = [=](label face)
        {
            foamAtomic::AtomicAdd(sumAPtr[uPtr[face]], dot(lowerPtr[face], localOne));
            foamAtomic::AtomicAdd(sumAPtr[lPtr[face]], dot(upperPtr[face], localOne));
        };
        exec.parallelFor(LambdaOffDiag, nFaces);
    }

    // Add the interface internal coefficients to diagonal
    // and the interface boundary coefficients to the sum-off-diagonal
    forAll(interfaces_, patchi)
    {
        if (interfaces_.set(patchi))
        {
            const labelUList& pa = lduAddr().patchAddr(patchi);
            const Field<LUType>& pCoeffs = interfacesUpper_[patchi];

            const auto paPtr = pa.cbegin();
            const auto pCoeffsPtr = pCoeffs.cbegin();

            auto Lambda = [=](label face)
            {
                foamAtomic::AtomicAdd(sumAPtr[paPtr[face]], -dot(pCoeffsPtr[face], localOne));
            };
            exec.parallelFor(Lambda, pa.size());
        }
    }
}


template<class Type, class DType, class LUType>
void Foam::LduMatrix<Type, DType, LUType>::residual
(
    Field<Type>& rA,
    const Field<Type>& psi
) const
{
    const auto& addr = lduAddr();

    Type* __restrict__ rAPtr = rA.begin();

    const Type* const __restrict__ psiPtr = psi.begin();
    const DType* const __restrict__ diagPtr = diag().begin();
    const Type* const __restrict__ sourcePtr = source().begin();

    const label* const __restrict__ uPtr = lduAddr().upperAddr().begin();
    const label* const __restrict__ lPtr = lduAddr().lowerAddr().begin();

    const LUType* const __restrict__ upperPtr = upper().begin();
    const LUType* const __restrict__ lowerPtr = lower().begin();

    // Parallel boundary initialisation.
    // Note: there is a change of sign in the coupled
    // interface update to add the contibution to the r.h.s.

    const label startRequest = UPstream::nRequests();

    foamExecutor exec;

    // Initialise the update of interfaced interfaces
    initMatrixInterfaces
    (
        false,          // negate interface contributions
        interfacesUpper_,
        psi,
        rA
    );

    const label nCells = diag().size();
    
    if (hasLowerCSR())
    {
        // Use cell-based looping
        if (debugLevel() == 2) PoutInFunction<< "cell-based looping" << endl;

        const label* const __restrict__ oStartPtr =
            addr.ownerStartAddr().begin();
        const label* const __restrict__ loStartPtr =
            addr.losortStartAddr().begin();
        const label* const __restrict__ lcsrPtr =
                addr.lowerCSRAddr().begin();

        // Note: lowerCSR constructed from lower if available, upper otherwise
        //       so is handling symmetric()
        const scalar* const __restrict__ lowercsrPtr = lowerCSR().begin();

        auto LambdaAmul = [=](label cell)
        {
            auto& val = rAPtr[cell];

            if constexpr(std::is_same<DType, Type>::value)
            {
                val = sourcePtr[cell] - cmptMultiply(diagPtr[cell], psiPtr[cell]);
            }
            else
            {
                val = sourcePtr[cell] - dot(diagPtr[cell], psiPtr[cell]);
            }

            // Add lower contributions
            {
                const label start = loStartPtr[cell];
                const label end = loStartPtr[cell+1];

                for (label i = start; i < end; i++)
                {
                    const label nbrCell = lcsrPtr[i];
                    val -= dot(lowercsrPtr[i], psiPtr[nbrCell]);
                }
            }
            // Add upper contributions
            {
                const label start = oStartPtr[cell];
                const label end = oStartPtr[cell+1];

                for (label i = start; i < end; i++)
                {
                    const label nbrCell = uPtr[i];
                    val -= dot(upperPtr[i], psiPtr[nbrCell]);
                }
            }
        };
        exec.parallelFor(LambdaAmul, nCells);
    }
    else
    {
        auto LambdaDiag = [=](label cell)
        {
            if constexpr(std::is_same<DType, Type>::value)
            {
                rAPtr[cell] = sourcePtr[cell] - cmptMultiply(diagPtr[cell], psiPtr[cell]);
            }
            else
            {
                rAPtr[cell] = sourcePtr[cell] - dot(diagPtr[cell], psiPtr[cell]);
            }
        };
        exec.parallelFor(LambdaDiag, nCells);

        const label nFaces = upper().size();

        auto LambdaOffDiag = [=](label face)
        {
            foamAtomic::AtomicAdd(rAPtr[uPtr[face]], -dot(lowerPtr[face], psiPtr[lPtr[face]]));
            foamAtomic::AtomicAdd(rAPtr[lPtr[face]], -dot(upperPtr[face], psiPtr[uPtr[face]]));
        };
        exec.parallelFor(LambdaOffDiag, nFaces);
    }

    // Update interface interfaces
    updateMatrixInterfaces
    (
        false,
        interfacesUpper_,
        psi,
        rA,
        startRequest
    );
}


template<class Type, class DType, class LUType>
Foam::tmp<Foam::Field<Type>> Foam::LduMatrix<Type, DType, LUType>::residual
(
    const Field<Type>& psi
) const
{
    auto trA = tmp<Field<Type>>::New(psi.size());
    residual(trA.ref(), psi);
    return trA;
}


// ************************************************************************* //
