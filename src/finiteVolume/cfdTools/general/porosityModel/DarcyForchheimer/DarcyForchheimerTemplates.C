/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012-2016 OpenFOAM Foundation
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

#include "deviceUtils.H"

// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

template<class RhoFieldType>
void Foam::porosityModels::DarcyForchheimer::apply
(
    scalarField& Udiag,
    vectorField& Usource,
    const scalarField& V,
    const RhoFieldType& rho,
    const scalarField& mu,
    const vectorField& U
) const
{
    forAll(cellZoneIDs_, zoneI)
    {
        const tensorField& dZones = D_[zoneI];
        const tensorField& fZones = F_[zoneI];

        const labelList& cells = mesh_.cellZones()[cellZoneIDs_[zoneI]];

        foamExecutor exec;
        const auto dZonesPtr = dZones.cbegin();
        const auto fZonesPtr = fZones.cbegin();
        const auto cellsPtr = cells.cbegin();
        const auto VPtr = V.cbegin();
        const auto UPtr = U.cbegin();
        const auto rhoPtr = argWrapper::cget(rho);
        const auto muPtr = mu.cbegin();
        auto UdiagPtr = Udiag.begin();
        auto UsourcePtr = Usource.begin();

        const bool isUniform = csysPtr_->uniform();
        
        const tensor localI(I);
        auto Lambda = [=](label i)
        {
            const label celli = cellsPtr[i];
            const label j = isUniform ? 0 : i;
            const tensor Cd =
                muPtr[celli]*dZonesPtr[j] + (rhoPtr[celli]*mag(UPtr[celli]))*fZonesPtr[j];

            const scalar isoCd = tr(Cd);

            UdiagPtr[celli] += VPtr[celli]*isoCd;
            UsourcePtr[celli] -= VPtr[celli]*((Cd - localI*isoCd) & UPtr[celli]);  
        };
        exec.parallelFor(Lambda, cells.size());
    }
}


template<class RhoFieldType>
void Foam::porosityModels::DarcyForchheimer::apply
(
    tensorField& AU,
    const RhoFieldType& rho,
    const scalarField& mu,
    const vectorField& U
) const
{
    forAll(cellZoneIDs_, zoneI)
    {
        const tensorField& dZones = D_[zoneI];
        const tensorField& fZones = F_[zoneI];

        const labelList& cells = mesh_.cellZones()[cellZoneIDs_[zoneI]];

        foamExecutor exec;
        const auto cellsPtr = cells.cbegin();
        const auto dZonesPtr = dZones.cbegin();
        const auto fZonesPtr = fZones.cbegin();
        const auto muPtr = mu.cbegin();
        const auto UPtr = U.cbegin();
        const auto rhoPtr = argWrapper::cget(rho);
        auto AUPtr = AU.begin();
        const bool isUniform = csysPtr_->uniform();

        auto Lambda = [=](label i)
        {
            const label celli = cellsPtr[i];
            const label j = isUniform ? 0 : i;
            const tensor D = dZonesPtr[j];
            const tensor F = fZonesPtr[j];

            AUPtr[celli] += muPtr[celli]*D + (rhoPtr[celli]*mag(UPtr[celli]))*F;
        };
        exec.parallelFor(Lambda, cells.size());

    }
}


// ************************************************************************* //
