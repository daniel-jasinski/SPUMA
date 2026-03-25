/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
    Copyright (C) 2015-2017 OpenCFD Ltd.
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
    Foam::deviceHePsiThermo

Description
    Energy for a mixture based on compressibility (on GPU).
    Temporary class to run compressible cases on GPU. Mixtures different
    from pureMixture are not supported.

SourceFiles
    deviceHePsiThermo.C

\*---------------------------------------------------------------------------*/

#ifndef deviceHePsiThermo_H
#define deviceHePsiThermo_H

#include "psiThermo.H"
#include "deviceHeThermo.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class deviceHePsiThermo Declaration
\*---------------------------------------------------------------------------*/

template<class BasicPsiThermo, class MixtureType>
class deviceHePsiThermo
:
    public deviceHeThermo<BasicPsiThermo, MixtureType>
{
    // Private Member Functions

        //- Calculate the thermo variables
        void calculate
        (
            const volScalarField& p,
            volScalarField& T,
            volScalarField& he,
            volScalarField& psi,
            volScalarField& mu,
            volScalarField& alpha,
            const bool doOldTimes
        );

        //- Construct as copy (not implemented)
        deviceHePsiThermo(const deviceHePsiThermo<BasicPsiThermo, MixtureType>&);


public:

    //- Runtime type information
    TypeName("deviceHePsiThermo");


    // Constructors

        //- Construct from mesh and phase name
        deviceHePsiThermo
        (
            const fvMesh&,
            const word& phaseName
        );

        //-  Construct from mesh and phase name and dict name
        deviceHePsiThermo
        (
            const fvMesh&,
            const word& phaseName,
            const word& dictionaryName
        );


    //- Destructor
    virtual ~deviceHePsiThermo();


    // Member functions

        //- Update properties
        virtual void correct();

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#ifdef NoRepository
    #include "deviceHePsiThermo.C"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
