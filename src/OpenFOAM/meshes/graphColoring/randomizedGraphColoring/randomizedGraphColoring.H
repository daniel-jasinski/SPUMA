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
    Foam::randomizedGraphColoring

Description
    Randomized graph coloring algorithm belonging to the class of Brooks-Vizing
    vertex coloring algorithms [Grable and Panconesi 2000] and described in
    Fratarcangeli, Marco & Tibaldo, Valentina & Pellacini, Fabio. (2016).
    Vivace: a Practical Gauss-Seidel Method for Stable Soft Body Dynamic.
    ACM Transactions on Graphics. 35. 10.1145/2980179.2982437.

Usage
    \table
        Property                       | Description                                               | Required    | Default value
        shrinkingFactorEvaluationMode  | Evaluation mode of the shrinking factor                   | no          | manual
        shrinkingFactor                | Reduction coefficient of the maximal degree of the graph  | no          | 1.0
        noProgressStreakThreshold      | Maximum number of iterations with no progress             | no          | 2
        maxIter                        | Maximum number of iterations of the main loop             | no          | 10000
        seed                           | User-defined seed for the random sequence                 | no          | 0
        log                            | Log verbosity                                             | no          | 0
        check                          | Check that correctness of the colouring                   | no          | false
    \endtable  

SourceFiles
    randomizedGraphColoring.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_randomizedGraphColoring_H
#define Foam_randomizedGraphColoring_H

#include "graphColoring.H"


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                       Class randomizedGraphColoring Declaration
\*---------------------------------------------------------------------------*/

class randomizedGraphColoring
: 
    public graphColoring
{
    // Private Data

        //- Evaluation mode of the shrinking factor (manual, auto)
        word shrinkingFactorEvaluationMode_;

        //- Reduction coefficient of the maximal degree of the graph
        scalar shrinkingFactor_;

        //- Maximum number of iterations with no progress
        label noProgressStreakThreshold_;

        //- Maximum number of iterations of the main loop
        label maxIter_;

        //- If true, use the same seed to initialize the random sequence
        bool deterministic_;

        //- User-defined seed for the random sequence  
        label seed_;

        //- Log verbosity
        label log_;

        //- Check that correctness of the colouring
        bool check_;


    // Private Member Functions

        //- No copy construct
        randomizedGraphColoring(const randomizedGraphColoring&) = delete;

        //- No copy assignment
        void operator=(const randomizedGraphColoring&) = delete;

        //- Check the output of the algorithm
        bool check(const List<label>& cellColors);

        //- Read algorithm control parameters
        void readControls();


public:

    // Static data

        // Declare name of the class and its debug switch
        ClassName("randomized");


    // Constructors

        //- Empty constructor
        randomizedGraphColoring
        (
            const dictionary& dict,
            const lduAddressing& addr,
            List<DynamicList<label>>& partitions
        ) :
            graphColoring
            (
                dict,
                addr,
                partitions
            )
        {
            readControls();
        }

    //- Destructor
    virtual ~randomizedGraphColoring() = default;


    // Member Functions

        // Operations

        //- Run the algorithm
        void execute();
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
