/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2024 AUTHOR,AFFILIATION
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
    Foam::cudalduExecutor

Description

SourceFiles
    cudalduExecutorI.H
    cudalduExecutor.C
    cudalduExecutorIO.C

\*---------------------------------------------------------------------------*/

#ifndef cudalduExecutor_H
#define cudalduExecutor_H

#include "label.H"
#include "refCount.H"
#include "lduAddressing.H"

#include "executorOps.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class Istream;
class Ostream;
template<typename Op> class cudalduExecutor;


/*---------------------------------------------------------------------------*\
                         Class cudalduExecutor Declaration
\*---------------------------------------------------------------------------*/
template<typename Op>
class cudalduExecutor
:
    public refCount
   // public lduExecutor;
{

    typedef typename Op::execResultT resultType;
    typedef typename Op::execT1 Type1;
    typedef typename Op::execT2 Type2;
    typedef typename Op::execT3 Type3;


    // Private Member Functions

        //- No copy construct
        cudalduExecutor(const cudalduExecutor&) = delete;

        //- No copy assignment
        void operator=(const cudalduExecutor&) = delete;


public:

    // Constructors

        //- Default construct
        cudalduExecutor() = default;


    // Selectors

        //- Select default constructed
        //static autoPtr<cudalduExecutor> New();


    // Member Functions
    void ownerLoop
    (
        resultType* cellArray,
        const Type1* faceArray,
        Op op,
        const label loop_len 
    ){};

    void neighbourLoop
    (        
        resultType* cellArray,
        const Type1* faceArray,
        Op op,
        const label loop_len
    ){};

    void ownNbrLoop
    (
        resultType* cellArray,
        const Type1* faceArray1,
        const Type2* faceArray2,
        Op op,
        const lduAddressing& lduAddr
    );


    // Member Operators

        //void operator=(const cudalduExecutor&);


    // Friend Functions

    // Friend Operators

    // IOstream Operators

        //friend Istream& operator>>(Istream&, cudalduExecutor&);
        //friend Ostream& operator<<(Ostream&, const cudalduExecutor&);
};

} // End namespace Foam

#ifdef NoRepository 
    #ifdef have_cuda
    #include "cudalduExecutor.cu"
    #endif
#endif


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
