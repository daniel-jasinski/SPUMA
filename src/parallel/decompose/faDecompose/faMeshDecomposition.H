/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2016-2017 Wikki Ltd
    Copyright (C) 2021,2025 OpenCFD Ltd.
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
    Foam::faMeshDecomposition

Description
    Automatic faMesh decomposition class

Author
    Zeljko Tukovic, FSB Zagreb
    Hrvoje Jasak, Wikki Ltd.

SourceFiles
    faMeshDecomposition.cxx

\*---------------------------------------------------------------------------*/

#ifndef Foam_faMeshDecomposition_H
#define Foam_faMeshDecomposition_H

#include "faMesh.H"

namespace Foam
{

/*---------------------------------------------------------------------------*\
                     Class faMeshDecomposition Declaration
\*---------------------------------------------------------------------------*/

class faMeshDecomposition
:
    public faMesh
{
    // Private Data

        //- The area-region name
        const word areaName_;

        //- Number of processors in decomposition
        const label nProcs_;

        //- Is the decomposition data to be distributed for each processor
        bool distributed_;

        //- Are globalFaceZones being used
        bool hasGlobalFaceZones_;

        //- Processor label for each cell
        labelList faceToProc_;

        //- Face labels for each processor mesh
        labelListList procFaceLabels_;

        //-
        List<Map<label>> procMeshEdgesMap_;

        //- Number of internal edges for each processor mesh
        labelList procNInternalEdges_;

        //- Edge labels for patches of processor meshes
        List<labelListList> procPatchEdgeLabels_;

        //- Labels of points for each processor
        labelListList procPatchPointAddressing_;

        //- Labels of edges for each processor
        labelListList procPatchEdgeAddressing_;

        //- Labels of edges for each processor
        labelListList procEdgeAddressing_;

        //- Labels of faces for each processor
        labelListList procFaceAddressing_;

        //- Original patch index for every processor patch
        labelListList procBoundaryAddressing_;

        //- Sizes for processor mesh patches
        // Excludes inter-processor boundaries
        labelListList procPatchSize_;

        //- Start indices for processor patches
        // Excludes inter-processor boundaries
        labelListList procPatchStartIndex_;

        //- Neighbour processor ID for inter-processor boundaries
        labelListList procNeighbourProcessors_;

        //- Sizes for inter-processor patches
        labelListList procProcessorPatchSize_;

        //- Start indices for inter-processor patches
        labelListList procProcessorPatchStartIndex_;

        //- List of globally shared point labels
        labelList globallySharedPoints_;

        //- Are there cyclic-parallel faces
        bool cyclicParallel_;


    // Private Member Functions

        void distributeFaces();

        //- No copy construct
        faMeshDecomposition(const faMeshDecomposition&) = delete;

        //- No copy assignment
        void operator=(const faMeshDecomposition&) = delete;

public:

    // Constructors

        //- Construct from components.
        //- Values will come from the volume decomposition
        faMeshDecomposition
        (
            //! The area-region name
            const word& areaName,
            //! The underlying polyMesh
            const polyMesh& mesh,
            //! The number of processors
            const label nProcessors,
            //! Additional parameters, sent to updateParameters
            const dictionary& params = dictionary::null
        );

        //- Construct from components (default area region).
        //- Values will come from the volume decomposition
        faMeshDecomposition
        (
            //! The underlying polyMesh
            const polyMesh& mesh,
            //! The number of processors
            const label nProcessors,
            //! Additional parameters, sent to updateParameters
            const dictionary& params = dictionary::null
        );


    //- Destructor
    ~faMeshDecomposition() = default;


    // Member Functions

    // Settings

        //- The area-region name
        const word& name() const noexcept { return areaName_; }

        //- Number of processor in decomposition
        label nProcs() const noexcept { return nProcs_; }

        //- Is decomposition data to be distributed for each processor
        bool distributed() const noexcept { return distributed_; }

        //- Change distributed flag
        bool distributed(bool on) noexcept
        {
            bool old(distributed_);
            distributed_ = on;
            return old;
        }

        //- Are global face zones used
        bool useGlobalFaceZones() const noexcept
        {
            return hasGlobalFaceZones_;
        }

        //- Change global face zones flag
        bool useGlobalFaceZones(bool on) noexcept
        {
            bool old(hasGlobalFaceZones_);
            hasGlobalFaceZones_ = on;
            return old;
        }

        //- Update flags based on the decomposition model settings
        //  Sets "distributed", detects presence of "globalFaceZones"
        void updateParameters(const dictionary& params);


    // Mappings

        //- Face-processor decomposition labels
        const labelList& faceToProc() const noexcept
        {
            return faceToProc_;
        }


    // Decompose

        //- Decompose mesh
        void decomposeMesh();

        //- Write decomposition
        bool writeDecomposition() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
