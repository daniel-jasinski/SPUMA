#ifndef Foam_cpu_Memory_Executor_H
#define Foam_cpu_Memory_Executor_H


#include "memoryExecutor.H"

namespace Foam
{
    

class cpuMemoryExecutor
: public memoryExecutor<cpuMemoryExecutor>
{
public:

    //- static data

    static constexpr label _memAlignBytes = 8;

    //- static member functions
    
    static void* _backendAlloc(uint64_t size)
    {
        return new char[size];
    };

    static void _backendClear(void* ptr)
    {
        delete [] static_cast<char*>(ptr);
    };

    static void _backendMemCopy(void* dst, const void* src, uint64_t size, memCopyKind kind)
    {
        std::memcpy(dst, src, static_cast<size_t>(size));
    };

    static void _backendMemSet(void* ptr, const uint64_t sizeInBytes, const void* value, size_t sizeOfValue)
    {
        char* tmpPtr = static_cast<char*>(ptr);
        const char* tmpValue = static_cast<const char*>(value);
        for (size_t i = 0; i < sizeInBytes; i+=sizeOfValue)
        {
            for (size_t j = 0; j < sizeOfValue; j++ )
            {
                tmpPtr[i+j] = tmpValue[j];
            }
        }
    };

    static void _backendMemSetScalarOne(void* ptr, const size_t sizeInBytes)
    {
        scalar* tmpPtr = static_cast<scalar*>(ptr);
        for (size_t i = 0; i < sizeInBytes/sizeof(scalar); i++)
        {
            tmpPtr[i] = 1.0;
        }
    };

    static void _backendMemSet(void* ptr, const uint64_t sizeInBytes, const int value)
    {
        std::memset(ptr, value, sizeInBytes);
    };

};


} // namespace Foam

#endif
