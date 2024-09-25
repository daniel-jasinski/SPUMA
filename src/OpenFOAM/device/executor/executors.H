#ifndef Foam_executors_H
#define Foam_executors_H

#include "executor.H"
#include "cpuExecutor.H"

#ifdef have_cuda
    #include "cudaExecutor.H"
#endif

#include "Atomic.H"

#include "deviceInit.H"

namespace Foam
{

//- choose backend
#ifdef have_cuda
    typedef cudaExecutor foamExecutor;
#else
    typedef cpuExecutor foamExecutor;
#endif

//- choose corresponding executor Atomic
#ifdef have_cuda
    typedef cudaAtomic foamAtomic;
#else
    typedef cpuAtomic foamAtomic;
#endif

// - choose corresponding init class
#ifdef have_cuda
    typedef cudaDeviceInit foamDeviceInit;
#else
    typedef cpuDeviceInit foamDeviceInit;
#endif
//fwd declaraction

// Macro for loops over cells with ldu addressing
#define forAllOwner(ownerStart,idCell,idFace,expression) \
    for (size_t idFace = ownStart[idCell]; idFace < ownStart[idCell+1]; idFace++)\
    {\
        expression\
    };
#define forAllNbr(losortStart,losort,idCell,nbrFace,expression)\
    for (size_t idlosort = losortStart[idCell]; idlosort < losortStart[idCell+1]; idlosort++)\
    {\
        const label nbrFace = losort[idlosort];\
        if(nbrFace != -1){\
            expression\
        };\
    };

#define parallelForAll(size,index,expression)\
    foamExecutor exec;\
    auto Lambda = [=](label index){expression};\
    exec.parallelFor(Lambda,size);

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
