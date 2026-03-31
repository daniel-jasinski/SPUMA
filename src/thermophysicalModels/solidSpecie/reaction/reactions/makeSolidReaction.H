/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2021 OpenCFD Ltd.
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

InClass
    Foam::makeSolidReaction

Description
    Macros for instantiating reactions for solid

\*---------------------------------------------------------------------------*/

#ifndef makeSolidReaction_H
#define makeSolidReaction_H

#include "solidReaction.H"
#include "IrreversibleReaction.H"
#include "Reaction.H"

#include "addToRunTimeSelectionTable.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#define makeSolidReaction(ReactionType, Thermo, ReactionRate)                  \
                                                                               \
    typedef solidReaction<Thermo> solidReaction##Thermo;                       \
                                                                               \
    typedef Reaction<Thermo> Reaction##Thermo;                                 \
                                                                               \
    typedef ReactionType<solidReaction, Thermo, ReactionRate>                  \
        ReactionType##Thermo##ReactionRate;                                    \
                                                                               \
    template<>                                                                 \
    const word ReactionType##Thermo##ReactionRate::typeName                    \
    (                                                                          \
        ReactionType::baseTypeName()                                              \
      + ReactionRate::type()                                                   \
      + solidReaction##Thermo::baseTypeName()                                     \
    );                                                                         \
                                                                               \
    /* Fix #48: Compute key directly from baseTypeName() / type() functions. */   \
    /* Use the fully-typed typedef (not bare ReactionType which is a class */  \
    /* template and requires template args outside template<> context). */     \
    addToRunTimeSelectionTableKey                                              \
    (                                                                          \
        Reaction##Thermo,                                                      \
        ReactionType##Thermo##ReactionRate,                                    \
        dictionary,                                                            \
        (word(ReactionType##Thermo##ReactionRate::baseTypeName())                 \
       + ReactionRate::type()                                                  \
       + solidReaction##Thermo::baseTypeName()).c_str()                           \
    );

#define makeSolidIRReactions(Thermo, ReactionRate)                             \
                                                                               \
    makeSolidReaction(IrreversibleReaction, Thermo, ReactionRate)


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
