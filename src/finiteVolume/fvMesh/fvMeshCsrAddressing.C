/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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

#include "fvMeshCsrAddressing.H"
#include "fvMesh.H"
#include "lduAddressing.H"
#include "MemoryPool.H"

// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //

// Constructor/Destructor with logging
Foam::fvMeshCsrAddressing::fvMeshCsrAddressing(const fvMesh& mesh)
: mesh_(mesh)
{
}


// * * * * * * * * * * * * * * * * Destructor  * * * * * * * * * * * * * * * //

Foam::fvMeshCsrAddressing::~fvMeshCsrAddressing()
{
    clear();
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

void Foam::fvMeshCsrAddressing::clear()
{
    offsetsPtr_.release();
    indicesPtr_.release();
    signsPtr_.release();
}


void Foam::fvMeshCsrAddressing::buildIfNeeded() const
{
    // Simple lazy-build: if the three arrays are present, assume up-to-date.
    // We intentionally do not call any non-existing lduAddressing::version()
    // accessor here; explicit invalidation via clear() is required if the
    // underlying lduAddressing is modified.
    if (offsetsPtr_ && indicesPtr_ && signsPtr_)
    {
        return;
    }

    build();
}


void Foam::fvMeshCsrAddressing::build() const
{
    const auto& cells = mesh_.cells();
    const labelUList& owner = mesh_.owner();
    const lduAddressing& base = mesh_.lduAddr();
    const label nCells    = base.size();
    const labelUList& nbr = base.upperAddr();           // neighbour per internal face
    const labelUList& ownStart = base.ownerStartAddr(); // CSR ranges for owner faces
    const label nIntFaces = nbr.size();
    const label nFaces = mesh_.nFaces();

    // 1) prefix-sum -> offsets (size nCells+1)
    // Allocate persistent arrays using poolSwitch(1) so they are compatible
    // with device kernels and other pool-allocated structures. Ownership is
    // held by unique_ptr members so they will be reset safely when clear()
    // is called.
    offsetsPtr_ = std::make_unique<labelList>
    (
        nCells + 1,
        Foam::zero{},
        poolSwitch(1)
    );
    auto& offs = *offsetsPtr_;

    for (label celli = 0; celli < nCells; ++celli)
    {
        offs[celli+1] = offs[celli] + cells[celli].size();
    }

    // 2) allocate flat arrays (nnz = offs[nCells])
    const label nnz = offs[nCells];

    // Allocate flat arrays using poolSwitch(1) as well and store in
    // unique_ptr members.
    indicesPtr_ = std::make_unique<labelList>(nnz, Foam::zero{}, poolSwitch(1));
    signsPtr_ = std::make_unique<labelList>(nnz, Foam::zero{}, poolSwitch(1));
    auto& idx = *indicesPtr_;
    auto& sgn = *signsPtr_;

    // 3) fill using write heads
    labelList head = offs;

    for (label celli = 0; celli < nCells; ++celli)
    {
        const auto& cFaces = cells[celli];

        for (const label facei : cFaces)
        {
            const label entryi = head[celli]++;
            idx[entryi] = facei;

            if (facei < nIntFaces)
            {
                // owner cells
                if (owner[facei] == celli)
                {
                    sgn[entryi] = 1;
                }
                else // neighbour cells
                {
                    sgn[entryi] = -1;
                }
            }
            else // boundary neighbour cells
            {
                sgn[entryi] = 1;
            }
        }
    }
}


// * * * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * //

const Foam::labelUList& Foam::fvMeshCsrAddressing::offsets() const
{
    buildIfNeeded();
    return *offsetsPtr_;
}


const Foam::labelUList& Foam::fvMeshCsrAddressing::indices() const
{
    buildIfNeeded();
    return *indicesPtr_;
}


const Foam::labelUList& Foam::fvMeshCsrAddressing::signs() const
{
    buildIfNeeded();
    return *signsPtr_;
}


// ************************************************************************* //

