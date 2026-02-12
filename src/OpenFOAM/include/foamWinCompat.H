/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    Windows MSVC compatibility: provide POSIX type definitions that
    are not available in the MSVC C runtime.

\*---------------------------------------------------------------------------*/

#ifndef Foam_WinCompat_H
#define Foam_WinCompat_H

#include <sys/types.h>

#ifdef _WIN32
#  ifndef _MODE_T_DEFINED
     typedef unsigned short mode_t;
#    define _MODE_T_DEFINED
#  endif
#  ifndef _PID_T_DEFINED
     typedef int pid_t;
#    define _PID_T_DEFINED
#  endif
#endif

#endif

// ************************************************************************* //
