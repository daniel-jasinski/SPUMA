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
    Foam::memoryRegister

Description

SourceFiles
    memoryRegisterI.H
    memoryRegister.C
    memoryRegisterIO.C

\*---------------------------------------------------------------------------*/

#ifndef Foam_memoryRegister_H
#define Foam_memoryRegister_H

#include "label.H"
#include "IOstreams.H"
#include "memoryRegisterMode.H"
#include <map>
#ifdef have_cuda
#include "cudaRegisterExecutor.cuh"
#endif
// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

// Forward Declarations
class Istream;
class Ostream;
class memoryRegister;

Ostream& operator<<(Ostream&, const memoryRegister&);


/*---------------------------------------------------------------------------*\
                         Class memoryRegister Declaration
\*---------------------------------------------------------------------------*/

class memoryRegister
{
        typedef std::map<void*, label> blockList;
        typedef std::pair<void*, label> blockPair;

    // Private Data

        //- unique register pointer
        static memoryRegister* v_;

        //- register executor
        #ifdef have_cuda
        cudaRegisterExecutor* exec_;
        #endif
        //- map of registered pointer
        blockList registerList_;

        //- total registered size
        uint64_t totalSize_; 
    
    // Private Member Functions
        //- Default construct
        memoryRegister();
        //- No copy construct
        memoryRegister(const memoryRegister&) = delete;

        //- No copy assignment
        void operator=(const memoryRegister&) = delete;



public:

    typedef memoryRegisterMode Mode;
    // Static Data Members

        //- register mode
        static Mode mode;
    // Selectors

        //- Select default constructed
        static memoryRegister* New();

    //- Destructor
    virtual ~memoryRegister();


    // Member Functions

        //- register memory for device to use
        inline void hostRegister(void* ptr, const label sizeInBytes, Mode mode = Mode::DEFAULT )
        {   
            if (this->isRegistered(ptr))
            {
                Info<< "ptr already registered" << endl;
                return;
            }
            
            exec_->memRegister(ptr,sizeInBytes,mode);

            registerList_.insert({ptr,sizeInBytes});
            totalSize_ += sizeInBytes;
        };

        //- unregister all memory
        inline void hostUnRegisterAll()
        {
            for (blockList::iterator ii = registerList_.begin(); ii!=registerList_.end(); ++ii)
            {
                exec_->memUnRegister(ii->first);
            };

            registerList_.clear();
            totalSize_ = 0;    
        };
        
        //- check if a pointer is registered
        inline bool isRegistered(void * ptr) const noexcept
        {
            bool registered =
               registerList_.find(ptr) == registerList_.end() ?
               false : true;
            
            return  registered;
        }

        //- get total registered size
        inline uint64_t totalSize() const noexcept
        {
            return totalSize_;
        }

    // Friend Functions

    // Friend Operators

    // IOstream Operators

        friend Ostream& operator<<(Ostream& os,const memoryRegister* memReg)
        {
            for (auto ii = memReg->registerList_.cbegin(); ii!=memReg->registerList_.cend(); ++ii)
            {
                os<< ii->first << " " << ii->second << nl;
            }
            return os;
        };
};


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

//#include "memoryRegisterI.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
