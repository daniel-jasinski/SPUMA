/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019 OpenCFD Ltd.
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

#include "adjustPhi.H"
#include "volFields.H"
#include "surfaceFields.H"
#include "inletOutletFvPatchFields.H"

// * * * * * * * * * * * * * * * Global Functions  * * * * * * * * * * * * * //

bool Foam::adjustPhi
(
    surfaceScalarField& phi,
    const volVectorField& U,
    volScalarField& p
)
{
    if (p.needReference())
    {
        scalar massIn = 0.0;
        scalar fixedMassOut = 0.0;
        scalar adjustableMassOut = 0.0;

        surfaceScalarField::Boundary& bphi =
            phi.boundaryFieldRef();

        forAll(bphi, patchi)
        {
            const fvPatchVectorField& Up = U.boundaryField()[patchi];
            const fvsPatchScalarField& phip = bphi[patchi];

            foamExecutor exec;            
            const auto phipPtr = phip.cbegin();
            auto LambdaIn = [=](label i)
            {
                scalar value = 0.0;
                if(phipPtr[i] < 0.0)
                    value = -phipPtr[i];
                return value;
            };
            auto LambdaOut = [=](label i)
            {   
                scalar value = 0.0;
                if(!(phipPtr[i] < 0.0))
                    value = phipPtr[i];
                return value;  
            };
            
            if (!phip.coupled())
            {
                if (Up.fixesValue() && !isA<inletOutletFvPatchVectorField>(Up))
                {
                    // forAll(phip, i)
                    // {
                    //     if (phip[i] < 0.0)
                    //     {
                    //         massIn -= phip[i];
                    //     }
                    //     else
                    //     {
                    //         fixedMassOut += phip[i];
                    //     }
                    // }
                    exec.reductionSum(LambdaIn,&massIn,phip.size());
                    exec.reductionSum(LambdaOut,&fixedMassOut,phip.size());
                }
                else
                {
                    // forAll(phip, i)
                    // {
                    //     if (phip[i] < 0.0)
                    //     {
                    //         massIn -= phip[i];
                    //     }
                    //     else
                    //     {
                    //         adjustableMassOut += phip[i];
                    //     }
                    // }
                    exec.reductionSum(LambdaIn,&massIn,phip.size());
                    exec.reductionSum(LambdaOut,&adjustableMassOut,phip.size());
                }
            }
        }

        // Calculate the total flux in the domain, used for normalisation
        scalar totalFlux = VSMALL + sum(mag(phi)).value();

        reduce(massIn, sumOp<scalar>());
        reduce(fixedMassOut, sumOp<scalar>());
        reduce(adjustableMassOut, sumOp<scalar>());

        scalar massCorr = 1.0;
        scalar magAdjustableMassOut = mag(adjustableMassOut);

        if
        (
            magAdjustableMassOut > VSMALL
         && magAdjustableMassOut/totalFlux > SMALL
        )
        {
            massCorr = (massIn - fixedMassOut)/adjustableMassOut;
        }
        else if (mag(fixedMassOut - massIn)/totalFlux > 1e-8)
        {
            FatalErrorInFunction
                << "Continuity error cannot be removed by adjusting the"
                   " outflow.\nPlease check the velocity boundary conditions"
                   " and/or run potentialFoam to initialise the outflow." << nl
                << "Total flux              : " << totalFlux << nl
                << "Specified mass inflow   : " << massIn << nl
                << "Specified mass outflow  : " << fixedMassOut << nl
                << "Adjustable mass outflow : " << adjustableMassOut << nl
                << exit(FatalError);
        }

        forAll(bphi, patchi)
        {
            const fvPatchVectorField& Up = U.boundaryField()[patchi];
            fvsPatchScalarField& phip = bphi[patchi];
            auto phipPtr = phip.begin();
            foamExecutor exec;
            if (!phip.coupled())
            {
                if
                (
                    !Up.fixesValue()
                 || isA<inletOutletFvPatchVectorField>(Up)
                )
                {
                    // forAll(phip, i)
                    // {
                    //     if (phip[i] > 0.0)
                    //     {
                    //         phip[i] *= massCorr;
                    //     }
                    // }
                    auto Lambda = [=](label i)
                    {
                        if (phipPtr[i] > 0.0)
                            phipPtr[i] *= massCorr; 
                    };
                    exec.parallelFor(Lambda,phip.size());
                }
            }
        }

        return mag(massIn)/totalFlux < SMALL
            && mag(fixedMassOut)/totalFlux < SMALL
            && mag(adjustableMassOut)/totalFlux < SMALL;
    }

    return false;
}


// ************************************************************************* //
