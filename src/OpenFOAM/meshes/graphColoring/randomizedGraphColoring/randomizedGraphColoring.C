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

\*---------------------------------------------------------------------------*/

#include "randomizedGraphColoring.H"
#include "addToRunTimeSelectionTable.H"


// * * * * * * * * * * * * * * Static Data Members * * * * * * * * * * * * * //

namespace Foam
{
    defineTypeNameAndDebug(randomizedGraphColoring, 0);
    addToRunTimeSelectionTable(graphColoring, randomizedGraphColoring, dictionary);
}


// * * * * * * * * * * * * * Private Member Functions  * * * * * * * * * * * //

bool Foam::randomizedGraphColoring::check
(
    const List<label>& cellColors
)
{
    const label* const __restrict__ uPtr =
        addr_.upperAddr().begin();
    const label* const __restrict__ lPtr =
        addr_.lowerAddr().begin();
    const label* const __restrict__ lcsrPtr =
        addr_.lowerCSRAddr().begin();

    const label* const __restrict__ ownStartPtr =
        addr_.ownerStartAddr().begin();
    const label* const __restrict__ losortStartPtr =
        addr_.losortStartAddr().begin();

    const label nCells = addr_.size();

    // Fill in the U set
    HashSet<label> U;
    for (label celli=0; celli<nCells; ++celli)
    {
        U.insert(celli);
    }

    // Check that all the neighbours of a cell have a different color
    for (const label& celli : U)
    {
        const label cellColor = cellColors[celli];

        // Upper check
        {
            const label start = ownStartPtr[celli]; 
            const label end = ownStartPtr[celli + 1];

            // check with the neighbour cells
            for (label i=start; i<end; ++i)
            {
                const label nbrCell = uPtr[i];
                if (cellColor == cellColors[nbrCell])
                {
                    return false;
                }
            }
        }
        
        // Lower check
        {
            const label start = losortStartPtr[celli];
            const label end = losortStartPtr[celli + 1];

            // check with the neighbour cells
            for (label i=start; i<end; ++i)
            {
                const label nbrCell = lcsrPtr[i];
                if (cellColor == cellColors[nbrCell])
                {
                    return false;
                }
            }
        }
    }

    return true;
}


void Foam::randomizedGraphColoring::readControls()
{
    shrinkingFactorEvaluationMode_ = dict_.getOrDefault<word>("shrinkingFactorEvaluationMode", "manual");
    shrinkingFactor_ = dict_.getOrDefault<scalar>("shrinkingFactor", 1.0);
    noProgressStreakThreshold_ = dict_.getOrDefault<label>("noProgressStreakThreshold", 2);
    maxIter_ = dict_.getOrDefault<label>("maxIter", 10000);
    deterministic_ = dict_.getOrDefault<bool>("deterministic", true);
    seed_ = dict_.getOrDefault<label>("seed", 0);
    log_ = dict_.getOrDefault<label>("log", 0);
    check_ = dict_.getOrDefault<bool>("check", false);
}


// * * * * * * * * * * * * * Public Member Functions  * * * * * * * * * * * * //

void Foam::randomizedGraphColoring::execute
(
)
{
    const label* const __restrict__ uPtr =
        addr_.upperAddr().begin();
    const label* const __restrict__ lPtr =
        addr_.lowerAddr().begin();
    const label* const __restrict__ lcsrPtr =
        addr_.lowerCSRAddr().begin();

    const label* const __restrict__ ownStartPtr =
        addr_.ownerStartAddr().begin();
    const label* const __restrict__ losortStartPtr =
        addr_.losortStartAddr().begin();

    const label nCells = addr_.size();

    // Initially, every node has a palette of size deltaV/shrinkingFactor_.
    // The maximum number of colors necessary for a graph coloring is deltaV + 1, 
    // but many graphs won't need that many colors. 
    // Therefore, we choose to shrink deltaV by a shrinking factor.
    // If the shrinking factor is too big, so that the problem is unsolvable,
    // then more colors will be added on the fly.

    label deltaV = 0;
    label shrinkingFactorInit = labelMax;

    // Compute max degree of a single cell and the shrinking factor.
    for (label celli=0; celli<nCells; ++celli)
    {
        label deltaVloc = 0;
        deltaVloc +=  ownStartPtr[celli+1] - ownStartPtr[celli];
        deltaVloc +=  losortStartPtr[celli+1] - losortStartPtr[celli];

        if (deltaVloc > deltaV)
            deltaV = deltaVloc;

        if (deltaVloc < shrinkingFactorInit)
            shrinkingFactorInit = deltaVloc;
    }

    if (shrinkingFactorEvaluationMode_ == "auto")
    {
        shrinkingFactor_ = shrinkingFactorInit;
    }

    // Maximum number of colors
    label maxColor = (deltaV + 1) / shrinkingFactor_;
    if (maxColor <= 0) 
    {
        maxColor = 1;
    }

    // Colors assigned to the cells.
    List<label> cellColors(nCells);

    // Next color of every node, in case the palette runs out.
    List<label> nextColor(nCells, maxColor);
    
    // Palettes to the cells.
    List<HashSet<label>> cellPalettes(nCells);

    // Initialize the palettes for all the cells.
    // The colors in the palette will be chosen randomly from, 
    // for all the remaining cells in U.
    for (label celli=0; celli<nCells; ++celli)
    {
        for (label j=0; j<maxColor; ++j)
        {
            cellPalettes[celli].insert(j);
        }
    }

    // Fill in the U set
    HashSet<label> U;
    for (label celli=0; celli<nCells; ++celli)
    {
        U.insert(celli);
    }

    // Use current time as seed for random generator (if deterministic_ is set to true)
    if (!deterministic_)
    {
        srand(time(0));
    }
    else
    {
        srand(seed_);
    }

    // Keep track of the number of iterations of the main loop.
    label iter = 0;

    // Keep track of the number of iterations with no progress.
    label noProgressStreak = 0;

    // If a node has found a color that solves the graph coloring for that node, 
    // then remove from U. Once U is empty, the graph coloring problem is done.
    while(U.size() > 0 && iter < maxIter_)
    {
        // Tentative coloring step
        // All remaining nodes in U are given a random color.
        for (const label& celli : U)
        {
            HashSet<label>& cellPalette = cellPalettes[celli];

            // Get random color from palette, and assign it.   
            int m = rand() % cellPalette.size();
            auto setIt = cellPalette.begin();
            std::advance(setIt, m);

            cellColors[celli] = *setIt;
        }

        HashSet<label> tmp;

        // Conflict resolution
        // Now let's find all the nodes whose colors are different from all their neighbours.
        // Those nodes will be removed from U, because they are done, 
        // with respect to the graph coloring problem.
        for (const label& celli : U)
        {
            const label cellColor = cellColors[celli];

            bool differentFromNeighbours = true;

            // Check if graph coloring property is solved for node.
            // Upper check
            {
                const label start = ownStartPtr[celli]; 
                const label end = ownStartPtr[celli + 1];

                for (label i=start; i<end; ++i)
                {
                    const label nbrCell = uPtr[i];
                    if (cellColor == cellColors[nbrCell])
                    {
                        differentFromNeighbours = false;
                        break;
                    }
                }
            }

            // lower check
            if (differentFromNeighbours)
            {
                const label start = losortStartPtr[celli];
                const label end = losortStartPtr[celli + 1];

                for (label i=start; i<end; ++i)
                {
                    const label nbrCell = lcsrPtr[i];
                    if (cellColor == cellColors[nbrCell])
                    {
                        differentFromNeighbours = false;
                        break;
                    }
                }
            }

            if (differentFromNeighbours) 
            {
                // Found the right color for this one, so remove from U.
                // Morevoer, the neighbours of celli can't use this color anymore,
                // so remove it from their palettes.
                // Upper contribution
                {
                    const label start = ownStartPtr[celli]; 
                    const label end = ownStartPtr[celli + 1];

                    for (label i=start; i<end; ++i)
                    {
                        const label nbrCell = uPtr[i];
                        HashSet<label>& cellPalette = cellPalettes[nbrCell];
                        cellPalette.erase(cellColor);
                    }
                }

                // Lower contribution
                {
                    const label start = losortStartPtr[celli];
                    const label end = losortStartPtr[celli + 1];

                    for (label i=start; i<end; ++i)
                    {
                        const label nbrCell = lcsrPtr[i];
                        HashSet<label>& cellPalette = cellPalettes[nbrCell];
                        cellPalette.erase(cellColor);
                    }
                }
            }
            else 
            {
                // not a correct color. don't remove from U.
                tmp.insert(celli);
            }
        }

        if (U.size() == tmp.size()) 
        {
            noProgressStreak++;

            // If no progress for too many iterations, we have no choice but to feed a random node.
            if (noProgressStreak > noProgressStreakThreshold_) 
            {
                label m = rand() % U.size();
                auto setIt = U.begin();
                std::advance(setIt, m);

                cellPalettes[*setIt].insert(nextColor[*setIt]++);

                noProgressStreak = 0;
            }
        }

        U = tmp;

        // Feed the hungry
        // if palette empty, we add more colors on the fly.
        // if we don't do this, the algorithm will get stuck in a loop.
        for (const label& celli : U)
        {
            if (cellPalettes[celli].size() == 0)
            {
                cellPalettes[celli].insert(nextColor[celli]++);
            }
        }

        iter++;
    }

    if (iter == maxIter_)
    {
        FatalErrorInFunction
            << "Maximum number of iterations reached in randomized graph coloring"
            << exit(FatalError);
    }

    if (check_)
    {
        if (!check(cellColors))
        {
            FatalErrorInFunction
                << "Graph coloring is not correct"
                << exit(FatalError);
        }
    }

    // Compute the maximum number of colors 
    // after the correct execution of the algorithm
    maxColor = max(nextColor);

    if (log_ > 0)
    {
        Info << "Number of cells: " << nCells << " - Number of colors: " << maxColor << nl;
    }

    // Finally, we collect all the partitions then.
    partitions_.resize(maxColor);
    for(int i=0;i<maxColor;++i)
    {
        DynamicList<label>* obj = new (partitions_.data_bytes() + i*sizeof(DynamicList<label>)) DynamicList<label>(16, poolSwitch(1));
    }

    for (label color=0; color<maxColor; ++color)
    {
        DynamicList<label>& partition = partitions_[color];
           
        // The first partition is all nodes that use color 0,
        // the second partition use color 1, and so on. 
        for (label celli=0; celli<nCells; ++celli)
        {
            if (cellColors[celli] == color)
            {
                partition.append(celli);   
            }
        }
    }
}


// ************************************************************************* //
