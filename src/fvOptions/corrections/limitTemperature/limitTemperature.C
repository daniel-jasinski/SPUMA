/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012-2017 OpenFOAM Foundation
    Copyright (C) 2018-2022 OpenCFD Ltd.
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

#include "limitTemperature.H"
#include "fvMesh.H"
#include "basicThermo.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{
    defineTypeNameAndDebug(limitTemperature, 0);
    addToRunTimeSelectionTable(option, limitTemperature, dictionary);
}
}


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::fv::limitTemperature::writeFileHeader(Ostream& os)
{
    writeHeaderValue(os, "Tmin", Foam::name(Tmin_));
    writeHeaderValue(os, "Tmax", Foam::name(Tmax_));
    writeCommented(os, "Time");
    writeTabbed(os, "nDampedCellsMin_[count]");
    writeTabbed(os, "nDampedCellsMin_[%]");
    writeTabbed(os, "nDampedCellsMax_[count]");
    writeTabbed(os, "nDampedCellsMax_[%]");

    os  << endl;

    writtenHeader_ = true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::limitTemperature::limitTemperature
(
    const word& name,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
    fv::cellSetOption(name, modelType, dict, mesh),
    writeFile(mesh, name, typeName, dict, false),
    Tmin_(0),
    Tmax_(0),
    phase_()
{
    if (isActive())
    {
        read(dict);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::fv::limitTemperature::read(const dictionary& dict)
{
    if (!(fv::cellSetOption::read(dict) && writeFile::read(dict)))
    {
        return false;
    }

    coeffs_.readEntry("min", Tmin_);
    coeffs_.readEntry("max", Tmax_);
    coeffs_.readIfPresent("phase", phase_);

    if (Tmax_ < Tmin_)
    {
        FatalIOErrorInFunction(dict)
            << "Minimum temperature limit cannot exceed maximum limit" << nl
            << "min = " << Tmin_ << nl
            << "max = " << Tmax_
            << exit(FatalIOError);
    }

    if (Tmin_ < 0)
    {
        FatalIOErrorInFunction(dict)
            << "Minimum temperature limit cannot be negative" << nl
            << "min = " << Tmin_
            << exit(FatalIOError);
    }

    // Set the field name to that of the energy
    // field from which the temperature is obtained
    const auto& thermo =
        mesh_.lookupObject<basicThermo>
        (
            IOobject::groupName(basicThermo::dictName, phase_)
        );

    fieldNames_.resize(1, thermo.he().name());

    fv::option::resetApplied();


    if (canResetFile())
    {
        resetFile(typeName);
    }

    if (canWriteHeader())
    {
        writeFileHeader(file());
    }


    return true;
}


void Foam::fv::limitTemperature::correct(volScalarField& he)
{
    const auto& thermo =
        mesh_.lookupObject<basicThermo>
        (
            IOobject::groupName(basicThermo::dictName, phase_)
        );

    scalarField Tmin(cells_.size(), Tmin_);
    scalarField Tmax(cells_.size(), Tmax_);

    scalarField heMin(thermo.he(thermo.p(), Tmin, cells_));
    scalarField heMax(thermo.he(thermo.p(), Tmax, cells_));

    scalarField& hec = he.primitiveFieldRef();

    const scalarField& T = thermo.T();

    scalar Tmin0 = min(T);
    scalar Tmax0 = max(T);

    // Count nTotCells ourselves
    // (maybe only applying on a subset)
    labelField nBelowMinFld(1,0);
    labelField nAboveMaxFld(1,0);
    const label nTotCells(returnReduce(cells_.size(), sumOp<label>()));

    foamExecutor exec;
    auto hecPtr = hec.begin();
    const auto heMinPtr = heMin.cbegin();
    const auto heMaxPtr = heMax.cbegin();
    auto nAboveMaxPtr = nAboveMaxFld.begin();
    auto nBelowMinPtr = nBelowMinFld.begin();
    const auto cellsPtr = cells_.cbegin();

    auto Lambda = [=](label i){
        const label celli = cellsPtr[i];
        if (hecPtr[celli] < heMinPtr[i])
        {
            hecPtr[celli] = heMinPtr[i];
            foamAtomic::AtomicAdd(nBelowMinPtr[0],1);
        }
        else if (hecPtr[celli] > heMaxPtr[i])
        {
            hecPtr[celli] = heMaxPtr[i];
            foamAtomic::AtomicAdd(nAboveMaxPtr[0],1);
        }
    };
    exec.parallelFor(Lambda,cells_.size());

    label nBelowMin = nBelowMinFld[0];
    label nAboveMax = nAboveMaxFld[0];
    reduce(nBelowMin, sumOp<label>());
    reduce(nAboveMax, sumOp<label>());

    reduce(Tmin0, minOp<scalar>());
    reduce(Tmax0, maxOp<scalar>());

    // Percent, max 2 decimal places
    const auto percent = [](scalar num, label denom) -> scalar
    {
        return (denom ? 1e-2*round(1e4*num/denom) : 0);
    };

    const scalar nBelowMinPercent = percent(nBelowMin, nTotCells);
    const scalar nAboveMaxPercent = percent(nAboveMax, nTotCells);

    Info<< type() << "=" << name_ << ", Type=Lower"
        << ", LimitedCells=" << nBelowMin
        << ", CellsPercent=" << nBelowMinPercent
        << ", Tmin=" << Tmin_
        << ", UnlimitedTmin=" << Tmin0
        << endl;

    Info<< type() << "=" << name_ << ", Type=Upper"
        << ", LimitedCells=" << nAboveMax
        << ", CellsPercent=" << nAboveMaxPercent
        << ", Tmax=" << Tmax_
        << ", UnlimitedTmax=" << Tmax0
        << endl;


    if (canWriteToFile())
    {
        file()
            << mesh_.time().timeOutputValue() << token::TAB
            << nBelowMin << token::TAB
            << nBelowMinPercent << token::TAB
            << nAboveMax << token::TAB
            << nAboveMaxPercent
            << endl;
    }


    // Handle boundaries in the case of 'all'
    labelField changedValuesFld(1,0);
	changedValuesFld[0] = (nBelowMin || nAboveMax);
    if (!cellSetOption::useSubMesh())
    {
        volScalarField::Boundary& bf = he.boundaryFieldRef();

        forAll(bf, patchi)
        {
            fvPatchScalarField& hep = bf[patchi];
            auto hepPtr = hep.begin();

            if (!hep.fixesValue())
            {
                const scalarField& pp = thermo.p().boundaryField()[patchi];

                scalarField Tminp(pp.size(), Tmin_);
                scalarField Tmaxp(pp.size(), Tmax_);

                scalarField heMinp(thermo.he(pp, Tminp, patchi));
                scalarField heMaxp(thermo.he(pp, Tmaxp, patchi));

                const auto heMinpPtr = heMinp.cbegin();
                const auto heMaxpPtr = heMaxp.cbegin();
                auto changedValuesPtr = changedValuesFld.begin();
                auto Lambda2 = [=](label facei){
                    if (hepPtr[facei] < heMinpPtr[facei])
                    {
                        hepPtr[facei] = heMinpPtr[facei];
                        changedValuesPtr[0] = 1;
                    }
                    else if (hepPtr[facei] > heMaxpPtr[facei])
                    {
                        hepPtr[facei] = heMaxpPtr[facei];
                        changedValuesPtr[0] = 1;
                    }
                };
                exec.parallelFor(Lambda2,hep.size());
            }
        }
    }

    bool changedValues = changedValuesFld[0];

    if (returnReduceOr(changedValues))
    {
        // We've changed internal values so give
        // boundary conditions opportunity to correct
        he.correctBoundaryConditions();
    }
}


// ************************************************************************* //
