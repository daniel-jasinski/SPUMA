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

\*---------------------------------------------------------------------------*/
#ifndef FoamCudalduExecutor_C
#define FoamCudalduExecutor_C

#include "cudalduExecutor.H"
#include "deviceM.H"
#include "cudaError.H"

// * * * * * * * * * * * * * * kernels * * * * * * * * * * * * * //
namespace Foam
{

//kernels
namespace cuda
{
    template<class T, class T1, class T2, class Op>
    __global__
    void ownNbrOpKernel
    (
        T* const __restrict__ cellArray,
        const T1* const __restrict__ faceArray1,
        const T2* const __restrict__ faceArray2,
        Op op,
        const label* const __restrict__ ownStart,
        const label* const __restrict__ losortStart,
        const label* const __restrict__ losort,
        const label ncells
    )
    {
                unsigned int id  = blockIdx.x *blockDim.x + threadIdx.x; // global id
        const unsigned int gridSize = blockDim.x*gridDim.x; //number of thread in a grid

        //strided loop
        while (id < ncells)
        {   
            //owner loop
            for (size_t i = ownStart[id]; i < ownStart[id+1]; i++)
            {
                op(cellArray[id],faceArray1[i]);   
            }

            //neighbour loop
            for (size_t j = losortStart[id]; j < losortStart[id+1]; j++)
            {
                const label nbrFace = losort[j];
                if(nbrFace != -1){  // -1 in losort means that the current face has no neighbour?
                op(cellArray[id],faceArray2[nbrFace]);
                };   
            }

            id+=gridSize;
        }
        
    };

} //namespace cuda
    
} // namespace Foam


// * * * * * * * * * * * * * * * * Constructors  * * * * * * * * * * * * * * //



// * * * * * * * * * * * * * * Member Functions  * * * * * * * * * * * * * * //
template<typename Op>
void Foam::cudalduExecutor<Op>::ownNbrLoop
(
    resultType* cellArray,
    const Type1* faceArray1,
    const Type2* faceArray2,
    Op op,
    const lduAddressing& lduAddr
)
{

    const label loop_len = lduAddr.size();
    const label numBlocks = SET_NUM_BLOCKS(loop_len);

    Foam::cuda::ownNbrOpKernel<resultType,Type1,Type2,Op>
    <<<numBlocks ,NUM_THREADS_PER_BLOCK>>>
    //<<<1,1>>>
    (
        cellArray,
        faceArray1,
        faceArray2,
        op,
        lduAddr.ownerStartAddr().begin(),
        lduAddr.losortStartAddr().begin(),
        lduAddr.losortAddr().begin(),
        loop_len
    );

    deviceSync(); 
    CHECK_LAST_CUDA_ERROR();

}

// * * * * * * * * * * * * * * Member Operators  * * * * * * * * * * * * * * //

/*void Foam::cudalduExecutor::operator=(const cudalduExecutor& rhs)
{
    if (this == &rhs)
    {
        return;  // Self-assignment is a no-op
    }
}*/


// * * * * * * * * * * * * * * Friend Functions  * * * * * * * * * * * * * * //

// * * * * * * * * * * * * * * * Friend Operators  * * * * * * * * * * * * * //


#endif
// ************************************************************************* //
