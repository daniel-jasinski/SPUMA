/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
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

Class
    Foam::graphColoring

Description
    Abstract base class for coloring algorithms.

SourceFiles
    graphColoring.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_graphColoring_H
#define Foam_graphColoring_H

#include "lduMesh.H"
#include "labelList.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                       Class graphColoring Declaration
\*---------------------------------------------------------------------------*/

class graphColoring
{
protected:

    // Protected Data

        //- Graph coloring properties
        dictionary dict_;

        //- mesh addressing
        const lduAddressing& addr_;

        //- Graph coloring
        List<DynamicList<label>>& partitions_;


    // Private Member Functions

        //- No copy construct
        graphColoring(const graphColoring&) = delete;

        //- No copy assignment
        void operator=(const graphColoring&) = delete;


public:

    // Static data

        // Declare name of the class and its debug switch
        ClassName("graphColoring");

        declareRunTimeSelectionTable
        (
            autoPtr,
            graphColoring,
            dictionary,
            (
                const dictionary& dict,
                const lduAddressing& addr,
                List<DynamicList<label>>& partitions
            ), 
            (
                dict,
                addr,
                partitions
            )
        );

    // Static method

        //- Build a child class from a dictionary
        static autoPtr<graphColoring> New
        (
            const dictionary& dict,
            const lduAddressing& addr,
            List<DynamicList<label>>& partitions
        );

    // Constructors

        //- Default constructor
        graphColoring
        (
            const dictionary& dict,
            const lduAddressing& addr,
            List<DynamicList<label>>& partitions
        ) :
            dict_(dict),
            addr_(addr),
            partitions_(partitions)
        {}

    //- Destructor
    virtual ~graphColoring() = default;


    // Member Functions

        // Operations

        //- Run the algorithm
        virtual void execute() = 0;
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
