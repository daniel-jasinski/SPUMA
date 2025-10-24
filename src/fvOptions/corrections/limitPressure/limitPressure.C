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

#include "limitPressure.H"
#include "fvMesh.H"
#include "basicThermo.H"
#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{
    defineTypeNameAndDebug(limitPressure, 0);
    addToRunTimeSelectionTable(option, limitPressure, dictionary);
}
}


// * * * * * * * * * * * * Protected Member Functions  * * * * * * * * * * * //

void Foam::fv::limitPressure::writeFileHeader(Ostream& os)
{
    writeHeaderValue(os, "pmin", Foam::name(pmin_));
    writeHeaderValue(os, "pmax", Foam::name(pmax_));
    writeCommented(os, "Time");
    writeTabbed(os, "nDampedCellsMin_[count]");
    writeTabbed(os, "nDampedCellsMin_[%]");
    writeTabbed(os, "nDampedCellsMax_[count]");
    writeTabbed(os, "nDampedCellsMax_[%]");

    os  << endl;

    writtenHeader_ = true;
}


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

Foam::fv::limitPressure::limitPressure
(
    const word& name,
    const word& modelType,
    const dictionary& dict,
    const fvMesh& mesh
)
:
    fv::cellSetOption(name, modelType, dict, mesh),
    writeFile(mesh, name, typeName, dict, false),
    pName_("p"),
    pmin_(0),
    pmax_(0)
{
    if (isActive())
    {
        read(dict);
    }
}


// * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * //

bool Foam::fv::limitPressure::read(const dictionary& dict)
{
    if (!(fv::cellSetOption::read(dict) && writeFile::read(dict)))
    {
        return false;
    }

    coeffs_.readEntry("min", pmin_);
    coeffs_.readEntry("max", pmax_);
    coeffs_.readIfPresent("p", pName_);

    fieldNames_.resize(1, pName_);

    if (pmax_ < pmin_)
    {
        FatalIOErrorInFunction(dict)
            << "Minimum Pressure limit cannot exceed maximum limit" << nl
            << "min = " << pmin_ << nl
            << "max = " << pmax_
            << exit(FatalIOError);
    }

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


void Foam::fv::limitPressure::correct(volScalarField& p)
{
    scalar pmin0 = min(p).value();
    scalar pmax0 = max(p).value();
    const scalar pmin = this->pmin_;
    const scalar pmax = this->pmax_;

    // Count nTotCells ourselves
    // (maybe only applying on a subset)
    labelField nBelowMinFld(1,0);
    labelField nAboveMaxFld(1,0);
    const label nTotCells(returnReduce(cells_.size(), sumOp<label>()));

    scalarField& pif = p.primitiveFieldRef();

    foamExecutor exec;
    const auto pifPtr = pif.begin();
    auto nAboveMaxPtr = nAboveMaxFld.begin();
    auto nBelowMinPtr = nBelowMinFld.begin();
    const auto cellsPtr = cells_.cbegin();

    auto Lambda = [=](label i)
    {
        const label celli = cellsPtr[i];
        if (pifPtr[celli] < pmin)
        {
            pifPtr[celli] = pmin;
            foamAtomic::AtomicAdd(nBelowMinPtr[0], 1);
        }
        else if (pifPtr[celli] > pmax)
        {
            pifPtr[celli] = pmax;
            foamAtomic::AtomicAdd(nAboveMaxPtr[0], 1);
        }
    };
    exec.parallelFor(Lambda, cells_.size());

    label nBelowMin = nBelowMinFld[0];
    label nAboveMax = nAboveMaxFld[0];
    reduce(nBelowMin, sumOp<label>());
    reduce(nAboveMax, sumOp<label>());

    reduce(pmin0, minOp<scalar>());
    reduce(pmax0, maxOp<scalar>());

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
        << ", pmin=" << pmin_
        << ", Unlimitedpmin=" << pmin0
        << endl;

    Info<< type() << "=" << name_ << ", Type=Upper"
        << ", LimitedCells=" << nAboveMax
        << ", CellsPercent=" << nAboveMaxPercent
        << ", pmax=" << pmax_
        << ", Unlimitedpmax=" << pmax0
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
    labelField changedValuesFld(1, 0);
    changedValuesFld[0] = (nBelowMin || nAboveMax);
    if (!cellSetOption::useSubMesh())
    {
        volScalarField::Boundary& bf = p.boundaryFieldRef();

        forAll(bf, patchi)
        {
            fvPatchScalarField& pp = bf[patchi];
            auto ppPtr = pp.begin();

            if (!pp.fixesValue())
            {
                auto changedValuesPtr = changedValuesFld.begin();
                auto Lambda2 = [=](label facei)
                {
                    if (ppPtr[facei] < pmin)
                    {
                        ppPtr[facei] = pmin;
                        changedValuesPtr[0] = 1;
                    }
                    else if (ppPtr[facei] > pmax)
                    {
                        ppPtr[facei] = pmax;
                        changedValuesPtr[0] = 1;
                    }
                };
                exec.parallelFor(Lambda2, pp.size());
            }
        }
    }

    bool changedValues = changedValuesFld[0];

    if (returnReduceOr(changedValues))
    {
        // We've changed internal values so give
        // boundary conditions opportunity to correct
        p.correctBoundaryConditions();
    }
}


// ************************************************************************* //
