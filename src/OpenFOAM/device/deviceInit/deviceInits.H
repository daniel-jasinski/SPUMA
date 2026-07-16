/*---------------------------------------------------------------------------*\
  *      .  *_______ * ______ .  __ *  __ * ___ .___    .  ___ .   *  .     *
    *  .    /       | |   _  \  |  |  |  | |   \/   | *   /   \ *   .    *   .
 *    .  * .\   (---*.|  |_)  |.|  |  |  |*|  \  /  |. * /  *  \  .  *     *
 =^^=^^==^^^=\   \^=^=|   ___/=^|  |^=|  |=|  |\/|  |^^=/  /=\  \^=^=^^===^^^=
 0  o  O  o---)   \ 0 |  |   0  |  o--o  |o|  |  |  | o/  _____  \ 0   o  O
     0    |_______/   |__| o   o \______/  |__| 0|__| /__/  o  \__\   o
  O   o  o        0  o      0   O        o    o       O  o     0   o    0  o
-------------------------------------------------------------------------------
    Copyright (C) 2025 Cineca
-------------------------------------------------------------------------------
License
    This file is part of SPUMA.

    SPUMA is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    SPUMA is distributed in the hope that it will be useful, but
    WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
    or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
    for more details.

    You should have received a copy of the GNU General Public License
    along with SPUMA.  If not, see <http://www.gnu.org/licenses/>.

Description
    Compile-time selection of the GPU initialization executor.

SourceFiles
    deviceInits.H
\*---------------------------------------------------------------------------*/

#ifndef Foam_deviceInits_H
#define Foam_deviceInits_H

#include "cpuDeviceInit.H"
#ifdef have_cuda
    #include "cudaDeviceInit.cuh"
#elif have_hip
    #include "hipDeviceInit.hpp"
#elif have_sycl
    #include "syclDeviceInit.H"
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

namespace Foam
{

#ifdef have_cuda
    typedef cudaDeviceInit foamDeviceInit;
#elif have_hip
    typedef hipDeviceInit foamDeviceInit;
#elif have_sycl
    typedef syclDeviceInit foamDeviceInit;
#else
    typedef cpuDeviceInit foamDeviceInit;
#endif


//- Shut down the selected device runtime after later-created objects have
//  been destroyed. Construct this guard immediately after device startup.
class deviceShutdownGuard
{
public:

    deviceShutdownGuard() noexcept = default;

    deviceShutdownGuard(const deviceShutdownGuard&) = delete;

    void operator=(const deviceShutdownGuard&) = delete;

    ~deviceShutdownGuard()
    {
        foamDeviceInit::shutdown();
    }
};

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

} // End namespace Foam


// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

//- Return \a code from an application main().
//  On Windows the process is terminated with TerminateProcess() after
//  shutting down the device runtime: the static-destructor / DLL-detach
//  chain crashes at exit due to cross-DLL destruction order, after all
//  application work (including result writing) has completed. std::_Exit()
//  is not sufficient: it reaches ExitProcess(), which still delivers
//  DLL_PROCESS_DETACH notifications. Skips the destructors of main()
//  locals; the device runtime is shut down explicitly instead of via
//  deviceShutdownGuard.
#ifdef _WIN32
    // Manual Win32 declarations: <windows.h> cannot be included in
    // acpp-compiled code (winnt.h conflicts with SYCL device compilation).
    extern "C" __declspec(dllimport) void* __stdcall GetCurrentProcess(void);
    extern "C" __declspec(dllimport) int __stdcall
        TerminateProcess(void* hProcess, unsigned int uExitCode);

    #define FOAM_EXIT_MAIN(code)                                               \
        do                                                                     \
        {                                                                      \
            ::Foam::foamDeviceInit::shutdown();                                \
            TerminateProcess(GetCurrentProcess(), (code));                     \
        } while (false)
#else
    #define FOAM_EXIT_MAIN(code) return (code)
#endif

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

#endif

// ************************************************************************* //
