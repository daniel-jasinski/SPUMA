#ifndef Foam_executors_H
#define Foam_executors_H

#include "executor.H"
#include "cpuExecutor.H"
#include "cudaExecutor.H"

namespace Foam
{

#ifdef have_cuda
    typedef cudaExecutor foamExecutor;
#else
    typedef cpuExecutor foamExecutor;
#endif

//fwd declaraction

// handy Macro
#define forAllOwner(ownerStart,idCell,idFace,...) \
    for (size_t idFace = ownStart[idCell]; idFace < ownStart[idCell+1]; idFace++)\
    {\
        __VA_ARGS__\
    };
#define forAllNbr(losortStart,losort,idCell,nbrFace,...)\
    for (size_t idlosort = losortStart[idCell]; idlosort < losortStart[idCell+1]; idlosort++)\
    {\
        const label nbrFace = losort[idlosort];\
        if(nbrFace != -1){\
            __VA_ARGS__\
        };\
    };

}; // namespace foam


    /*auto Lambda = [=](label id){
            
            //owner loop
            for (size_t i = ownStart[id]; i < ownStart[id+1]; i++)
            {
                diag[id] += lower[i];  
            }

            //neighbour loop
            for (size_t j = losortStart[id]; j < losortStart[id+1]; j++)
            {
                const label nbrFace = losort[j];
                if(nbrFace != -1){  // -1 in losort means that the current face has no neighbour
                    diag[id]+= upper[nbrFace];  
                };   
            }
    };*/

#endif
