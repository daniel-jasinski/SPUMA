/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2012-2017 OpenFOAM Foundation
    Copyright (C) 2020-2022 OpenCFD Ltd.
    Copyright (C) 2023-2025 Cineca
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
    Foam::fv::limitPressure

Group
    grpFvOptionsCorrections

Description
    Corrects Pressure field (i.e. \c p) within a specified
    region by applying limits between a given minimum and maximum.

    Corrections applied to:
    \verbatim
      p    | Pressure                   [Pa]
    \endverbatim

    Required fields:
    \verbatim
      p    | Pressure                   [Pa]
    \endverbatim

Usage
    Minimal example by using \c constant/fvOptions:
    \verbatim
    limitPressure1
    {
        // Mandatory entries (unmodifiable)
        type            limitPressure;

        // Mandatory entries (runtime modifiable)
        min             <minValue>;
        max             <maxValue>;

        // Mandatory/Optional (inherited) entries
        ...
    }
    \endverbatim

    where the entries mean:
    \table
      Property  | Description                 | Type   | Reqd  | Dflt
      type      | Type name: limitPressure    | word   | yes   | -
      min       | Minimum Pressure limit [Pa] | scalar | yes   | -
      max       | Maximum Pressure limit [Pa] | scalar | yes   | -
    \endtable

    The inherited entries are elaborated in:
      - \link fvOption.H \endlink
      - \link cellSetOption.H \endlink
      - \link writeFile.H \endlink

See also
  - Foam::fv::fixedPressureConstraint

SourceFiles
    limitPressure.C

\*---------------------------------------------------------------------------*/

#ifndef fv_limitPressure_H
#define fv_limitPressure_H

#include "cellSetOption.H"
#include "writeFile.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{
namespace fv
{

/*---------------------------------------------------------------------------*\
                    Class limitPressure Declaration
\*---------------------------------------------------------------------------*/

class limitPressure
:
    public fv::cellSetOption,
    public functionObjects::writeFile
{
protected:

    // Protected Data

        //- Name of operand pressure field
        word pName_;

        //- Minimum Pressure limit [K]
        scalar pmin_;

        //- Maximum Pressure limit [K]
        scalar pmax_;

    // Protected Member Functions

        //- Write file header information
        void writeFileHeader(Ostream& os);


public:

    //- Runtime type information
    TypeName("limitPressure");


    // Constructors

        //- Construct from components
        limitPressure
        (
            const word& name,
            const word& modelType,
            const dictionary& dict,
            const fvMesh& mesh
        );

        //- No copy construct
        limitPressure(const limitPressure&) = delete;

        //- No copy assignment
        void operator=(const limitPressure&) = delete;


    //- Destructor
    virtual ~limitPressure() = default;


    // Member Functions

        //- Read dictionary
        virtual bool read(const dictionary& dict);

        //- Correct the energy field
        virtual void correct(volScalarField& he);
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace fv
} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
