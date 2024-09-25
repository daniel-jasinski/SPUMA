#ifndef Foam_memory_Executor_H
#define Foam_memory_Executor_H

#include "label.H"
#include "scalar.H"
#include "memCopyKind.H"


namespace Foam
{
    

template<typename T>
class memoryExecutor
{

public:
    
    //memoryExecutor() = delete;
    //virtual ~memoryExecutor() {};
    // No copy constructor
    memoryExecutor(memoryExecutor<T> &other) = delete;   
    
    static inline label memAlignBytes()
    {
        return T::_memAlignBytes;
    };

    static inline void* alloc(uint64_t size)
    {
        return T::_backendAlloc(size);
    };

    static inline void clear(void* ptr)
    {
        T::_backendClear(ptr);
    };

    static inline void memCopy
    (
        void* dst,
        const void* src,
        uint64_t size,
        memCopyKind kind
    )
    {
        T::_backendMemCopy(dst, src, size, kind);
    };

    static inline void memSet
    (
        void* ptr,
        const size_t sizeInBytes,
        const void* value,
        size_t sizeOfValue
    )
    {
        T::_backendMemSet(ptr, sizeInBytes, value, sizeOfValue);
    };

    static inline void memSetScalarOne(void* ptr, const size_t sizeInBytes)
    {
       T::_backendMemSetScalarOne(ptr, sizeInBytes); 
    };

    static inline void memSet(void* ptr, const size_t sizeInBytes, const int value)
    {
        T::_backendMemSet(ptr,sizeInBytes,value);
    };

    //- operators

    void operator=(const memoryExecutor<T> &) = delete;

};


} // namespace Foam

#endif

