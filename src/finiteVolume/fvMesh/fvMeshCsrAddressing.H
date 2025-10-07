/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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

Class
    Foam::fvMeshCsrAddressing

Description
    CSR representation of the mesh addressing.

SourceFiles
    fvMeshCsrAddressing.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_fvMeshCsrAddressing_H
#define Foam_fvMeshCsrAddressing_H

#include "labelList.H"
#include "UList.H"
#include <memory>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class fvMesh;

/*---------------------------------------------------------------------------*\
                        Class fvMeshCsrAddressing Declaration
\*---------------------------------------------------------------------------*/

class fvMeshCsrAddressing
{
    // Private Data

        //- Reference to the mesh
        const fvMesh& mesh_;

        //- CSR offsets
        mutable std::unique_ptr<labelList> offsetsPtr_;
    
        //- CSR indices
        mutable std::unique_ptr<labelList> indicesPtr_;
    
        //- CSR signs
        mutable std::unique_ptr<labelList> signsPtr_;


    // Private Member Functions

        //- No copy construct
        fvMeshCsrAddressing(const fvMeshCsrAddressing&) = delete;

        //- No copy assignment
        void operator=(const fvMeshCsrAddressing&) = delete;

        //- Clear the addressing
        void clear();

        //- Build if needed
        void buildIfNeeded() const;

        //- Build the addressing
        void build() const;


public:

    // Constructors

        //- Construct from mesh
        explicit fvMeshCsrAddressing(const fvMesh& mesh);


    //- Destructor
        ~fvMeshCsrAddressing();


    // Member Functions

        //- Return CSR offsets
        const labelUList& offsets() const;

        //- Return CSR indices
        const labelUList& indices() const;

        //- Return CSR signs
        const labelUList& signs() const;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
