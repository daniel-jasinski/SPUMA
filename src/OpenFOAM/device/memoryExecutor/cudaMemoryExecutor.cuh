#ifndef Foam_cuda_Memory_Executor_H
#define Foam_cuda_Memory_Executor_H

#include "memoryExecutor.H"


namespace Foam
{

class cudaMemoryExecutor
: public memoryExecutor<cudaMemoryExecutor>
{
public:


    static constexpr label _memAlignBytes = 16;

    static void* _backendAlloc(uint64_t size);

    static void _backendClear(void* ptr);

    static void _backendMemCopy
    (
        void* dst,
        const void* src,
        uint64_t size,
        memCopyKind kind
    );

    static void _backendMemSet
    (
        void* ptr,
        const size_t sizeInBytes,
        const void* value,
        size_t sizeOfValue
    );

    static void _backendMemSetScalarOne(void* ptr, const size_t sizeInBytes);

    static void _backendMemSet(void* ptr, const size_t sizeInBytes, const int value);
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif
