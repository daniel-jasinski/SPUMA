/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2019-2022 OpenCFD Ltd.
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

#include "nutURoughWallFunctionFvPatchScalarField.H"
#include "turbulenceModel.H"
#include "fvPatchFieldMapper.H"
#include "volFields.H"
#include "addToRunTimeSelectionTable.H"


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

Foam::tmp<Foam::scalarField>
Foam::nutURoughWallFunctionFvPatchScalarField::calcNut() const
{
    const label patchi = patch().index();

    const auto& turbModel = db().lookupObject<turbulenceModel>
    (
        IOobject::groupName
        (
            turbulenceModel::propertiesName,
            internalField().group()
        )
    );

    const scalarField& y = turbModel.y()[patchi];

    const fvPatchVectorField& Uw = U(turbModel).boundaryField()[patchi];

    const tmp<scalarField> tnuw = turbModel.nu(patchi);
    const scalarField& nuw = tnuw();

    // The flow velocity at the adjacent cell centre
    const scalarField magUp(mag(Uw.patchInternalField() - Uw));

    const scalar yPlusLam = wallCoeffs_.yPlusLam();

    tmp<scalarField> tyPlus = calcYPlus(magUp);
    scalarField& yPlus = tyPlus.ref();

    auto tnutw = tmp<scalarField>::New(patch().size(), Zero);
    auto& nutw = tnutw.ref();

    foamExecutor exec;
    const auto yPlusPtr = yPlus.cbegin();
    const auto magUpPtr = magUp.cbegin();
    const auto yPtr = y.cbegin();
    const auto nuwPtr = nuw.cbegin();
    auto nutwPtr = nutw.begin();

    auto Lambda = [=](label facei)
    {
        if (yPlusLam < yPlusPtr[facei])
        {
            const scalar Re = magUpPtr[facei]*yPtr[facei]/nuwPtr[facei] + ROOTVSMALL;
            nutwPtr[facei] = nuwPtr[facei]*(sqr(yPlusPtr[facei])/Re - 1);
        }
    };
    exec.parallelFor(Lambda, yPlus.size());

    return tnutw;
}


Foam::tmp<Foam::scalarField>
Foam::nutURoughWallFunctionFvPatchScalarField::calcYPlus
(
    const scalarField& magUp
) const
{
    const label patchi = patch().index();

    const auto& turbModel = db().lookupObject<turbulenceModel>
    (
        IOobject::groupName
        (
            turbulenceModel::propertiesName,
            internalField().group()
        )
    );

    const scalarField& y = turbModel.y()[patchi];

    const tmp<scalarField> tnuw = turbModel.nu(patchi);
    const scalarField& nuw = tnuw();

    const scalar kappa = wallCoeffs_.kappa();
    const scalar E = wallCoeffs_.E();
    const scalar yPlusLam = wallCoeffs_.yPlusLam();

    auto tyPlus = tmp<scalarField>::New(patch().size(), Zero);
    auto& yPlus = tyPlus.ref();

    foamExecutor exec;
    auto yPlusPtr = yPlus.begin();
    const auto magUpPtr = magUp.cbegin();
    const auto nuwPtr = nuw.cbegin();
    const auto yPtr = y.cbegin();
    const scalar roughnessHeight = this->roughnessHeight_;
    const scalar roughnessFactor = this->roughnessFactor_;
    const scalar roughnessConstant = this->roughnessConstant_;
    const scalar tolerance = this->tolerance_;
    const label maxIter = this->maxIter_;
     
    if (roughnessHeight_ > 0.0)
    {
        // Rough Walls
        const scalar c_1 = 1.0/(90.0 - 2.25) + roughnessConstant_;
        const scalar c_2 = 2.25/(90.0 - 2.25);
        const scalar c_3 = 2.0*atan(1.0)/log(90.0/2.25);
        const scalar c_4 = c_3*log(2.25);

        {
            // If KsPlus is based on YPlus the extra term added to the law
            // of the wall will depend on yPlus
            auto Lambda = [=](label facei)
            {
                const scalar magUpara = magUpPtr[facei];
                const scalar Re = magUpara*yPtr[facei]/nuwPtr[facei];
                const scalar kappaRe = kappa*Re;

                scalar yp = yPlusLam;
                const scalar ryPlusLam = 1.0/yp;

                int iter = 0;
                scalar yPlusLast = 0.0;
                scalar dKsPlusdYPlus = roughnessHeight/yPtr[facei];

                // Additional tuning parameter - nominally = 1
                dKsPlusdYPlus *= roughnessFactor;
  
                do
                {
                    yPlusLast = yp;

                    // The non-dimensional roughness height
                    const scalar KsPlus = yp*dKsPlusdYPlus;

                    // The extra term in the law-of-the-wall
                    scalar G = 0;

                    scalar yPlusGPrime = 0;

                    if (KsPlus >= 90.0)
                    {
                        const scalar t_1 = 1 + roughnessConstant*KsPlus;
                        G = log(t_1);
                        yPlusGPrime = roughnessConstant*KsPlus/t_1;
                    }
                    else if (KsPlus > 2.25)
                    {
                        const scalar t_1 = c_1*KsPlus - c_2;
                        const scalar t_2 = c_3*log(KsPlus) - c_4;
                        const scalar sint_2 = sin(t_2);
                        const scalar logt_1 = log(t_1);
                        G = logt_1*sint_2;
                        yPlusGPrime =
                            (c_1*sint_2*KsPlus/t_1) + (c_3*logt_1*cos(t_2));
                    }

                    const scalar denom = 1.0 + log(E*yp) - G - yPlusGPrime;
                    if (mag(denom) > VSMALL)
                    {
                        yp = (kappaRe + yp*(1 - yPlusGPrime))/denom;
                    }
                } while
                (
                    mag(ryPlusLam*(yp - yPlusLast)) > tolerance
                 && ++iter < maxIter
                 && yp > VSMALL
                );

                yPlusPtr[facei] = max(scalar(0), yp);
            };
            exec.parallelFor(Lambda, yPlus.size());
        }
    }
    else
    {
        // Smooth Walls
        auto Lambda = [=](label facei)
        {
            const scalar magUpara = magUpPtr[facei];
            const scalar Re = magUpara*yPtr[facei]/nuwPtr[facei];
            const scalar kappaRe = kappa*Re;

            scalar yp = yPlusLam;
            const scalar ryPlusLam = 1.0/yp;

            int iter = 0;
            scalar yPlusLast = 0;

            do
            {
                yPlusLast = yp;
                yp = (kappaRe + yp)/(1.0 + log(E*yp));

            }
            while
            (
                mag(ryPlusLam*(yp - yPlusLast)) > tolerance
             && ++iter < maxIter
            );

            yPlusPtr[facei] = max(scalar(0), yp);
        };
        exec.parallelFor(Lambda, yPlus.size());
    }

    return tyPlus;
}


void Foam::nutURoughWallFunctionFvPatchScalarField::writeLocalEntries
(
    Ostream& os
) const
{
    os.writeEntry("roughnessHeight", roughnessHeight_);
    os.writeEntry("roughnessConstant", roughnessConstant_);
    os.writeEntry("roughnessFactor", roughnessFactor_);
    os.writeEntryIfDifferent<label>("maxIter", 10, maxIter_);
    os.writeEntryIfDifferent<scalar>("tolerance", 0.0001, tolerance_);
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::nutURoughWallFunctionFvPatchScalarField::
nutURoughWallFunctionFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF
)
:
    nutWallFunctionFvPatchScalarField(p, iF),
    roughnessHeight_(Zero),
    roughnessConstant_(Zero),
    roughnessFactor_(Zero),
    maxIter_(10),
    tolerance_(0.0001)
{}


Foam::nutURoughWallFunctionFvPatchScalarField::
nutURoughWallFunctionFvPatchScalarField
(
    const nutURoughWallFunctionFvPatchScalarField& ptf,
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const fvPatchFieldMapper& mapper
)
:
    nutWallFunctionFvPatchScalarField(ptf, p, iF, mapper),
    roughnessHeight_(ptf.roughnessHeight_),
    roughnessConstant_(ptf.roughnessConstant_),
    roughnessFactor_(ptf.roughnessFactor_),
    maxIter_(ptf.maxIter_),
    tolerance_(ptf.tolerance_)
{}


Foam::nutURoughWallFunctionFvPatchScalarField::
nutURoughWallFunctionFvPatchScalarField
(
    const fvPatch& p,
    const DimensionedField<scalar, volMesh>& iF,
    const dictionary& dict
)
:
    nutWallFunctionFvPatchScalarField(p, iF, dict),
    roughnessHeight_(dict.get<scalar>("roughnessHeight")),
    roughnessConstant_(dict.get<scalar>("roughnessConstant")),
    roughnessFactor_(dict.get<scalar>("roughnessFactor")),
    maxIter_(dict.getOrDefault<label>("maxIter", 10)),
    tolerance_(dict.getOrDefault<scalar>("tolerance", 0.0001))
{}


Foam::nutURoughWallFunctionFvPatchScalarField::
nutURoughWallFunctionFvPatchScalarField
(
    const nutURoughWallFunctionFvPatchScalarField& rwfpsf
)
:
    nutWallFunctionFvPatchScalarField(rwfpsf),
    roughnessHeight_(rwfpsf.roughnessHeight_),
    roughnessConstant_(rwfpsf.roughnessConstant_),
    roughnessFactor_(rwfpsf.roughnessFactor_),
    maxIter_(rwfpsf.maxIter_),
    tolerance_(rwfpsf.tolerance_)
{}


Foam::nutURoughWallFunctionFvPatchScalarField::
nutURoughWallFunctionFvPatchScalarField
(
    const nutURoughWallFunctionFvPatchScalarField& rwfpsf,
    const DimensionedField<scalar, volMesh>& iF
)
:
    nutWallFunctionFvPatchScalarField(rwfpsf, iF),
    roughnessHeight_(rwfpsf.roughnessHeight_),
    roughnessConstant_(rwfpsf.roughnessConstant_),
    roughnessFactor_(rwfpsf.roughnessFactor_),
    maxIter_(rwfpsf.maxIter_),
    tolerance_(rwfpsf.tolerance_)
{}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

Foam::tmp<Foam::scalarField>
Foam::nutURoughWallFunctionFvPatchScalarField::yPlus() const
{
    const label patchi = patch().index();

    const auto& turbModel = db().lookupObject<turbulenceModel>
    (
        IOobject::groupName
        (
            turbulenceModel::propertiesName,
            internalField().group()
        )
    );

    const fvPatchVectorField& Uw = U(turbModel).boundaryField()[patchi];
    tmp<scalarField> magUp = mag(Uw.patchInternalField() - Uw);

    return calcYPlus(magUp());
}


void Foam::nutURoughWallFunctionFvPatchScalarField::write
(
    Ostream& os
) const
{
    nutWallFunctionFvPatchScalarField::write(os);
    writeLocalEntries(os);
    fvPatchField<scalar>::writeValueEntry(os);
}


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
    makePatchTypeField
    (
        fvPatchScalarField,
        nutURoughWallFunctionFvPatchScalarField
    );
}

// ************************************************************************* //
