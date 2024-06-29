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
    Foam::cudaRegisterExecutor

Description

SourceFiles
    cudaRegisterExecutor.cuh

\*---------------------------------------------------------------------------*/

#ifndef Foam_cudaRegisterExecutor_H
#define Foam_cudaRegisterExecutor_H

#include "cudaError.H"
#include "label.H"
#include "memoryRegisterMode.H"
#include <cuda_runtime_api.h>

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

/*---------------------------------------------------------------------------*\
                         Class cudaRegisterExecutor Declaration
\*---------------------------------------------------------------------------*/

class cudaRegisterExecutor
{

    // Private Member Functions

        //- No copy construct
        cudaRegisterExecutor(const cudaRegisterExecutor&) = delete;

        //- No copy assignment
        void operator=(const cudaRegisterExecutor&) = delete;


public:

    typedef memoryRegisterMode Mode;
    // Constructors

        //- Default construct
        cudaRegisterExecutor() = default;

    //- Destructor
    ~cudaRegisterExecutor() = default;


    // Member Functions


    inline void memRegister(void* ptr, uint64_t sizeInBytes, Mode mode)
    {
        if ( mode == Mode::DEFAULT )
        {
            CHECK_CUDA_ERROR(cudaHostRegister(ptr,sizeInBytes,cudaHostRegisterDefault));
        }
        else if( mode == Mode::READONLY)
        {    
            CHECK_CUDA_ERROR(cudaHostRegister(ptr,sizeInBytes,cudaHostRegisterReadOnly));
        }
        else
        {
            Info<<"register method not implemented"<<endl;
            NotImplemented;
        }
    }

    inline void memUnRegister(void* ptr)
    {
        CHECK_CUDA_ERROR(cudaHostUnregister(ptr));
    }

};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
