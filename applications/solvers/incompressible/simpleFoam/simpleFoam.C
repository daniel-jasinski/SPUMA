/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2017 OpenFOAM Foundation
    Copyright (C) 2025 Cineca
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

Application
    simpleFoam

Group
    grpIncompressibleSolvers

Description
    Steady-state solver for incompressible, turbulent flows.

    \heading Solver details
    The solver uses the SIMPLE algorithm to solve the continuity equation:

        \f[
            \div \vec{U} = 0
        \f]

    and momentum equation:

        \f[
            \div \left( \vec{U} \vec{U} \right) - \div \gvec{R}
          = - \grad p + \vec{S}_U
        \f]

    Where:
    \vartable
        \vec{U} | Velocity
        p       | Pressure
        \vec{R} | Stress tensor
        \vec{S}_U | Momentum source
    \endvartable

    \heading Required fields
    \plaintable
        U       | Velocity [m/s]
        p       | Kinematic pressure, p/rho [m2/s2]
        \<turbulence fields\> | As required by user selection
    \endplaintable

\*---------------------------------------------------------------------------*/

#include <cstdio>
#include "fvCFD.H"
#include "dynamicFvMesh.H"
#include "singlePhaseTransportModel.H"
#include "turbulentTransportModel.H"
#include "simpleControl.H"
#include "fvOptions.H"

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

int main(int argc, char *argv[])
{
    argList::addNote
    (
        "Steady-state solver for incompressible, turbulent flows."
    );

    #include "postProcess.H"

    #include "addCheckCaseOptions.H"
    #include "setRootCaseLists.H"
    foamDeviceInit::Init();
    #include "createMemoryPool.H"
    #include "createTime.H"
    #include "createDynamicFvMesh.H"
    #include "createControl.H"
    #include "createFields.H"
    #include "initContinuityErrs.H"

    std::fprintf(stderr, "TRACE:loop 0 - before validate\n"); std::fflush(stderr);
    turbulence->validate();
    std::fprintf(stderr, "TRACE:loop 0a - after validate\n"); std::fflush(stderr);

    // * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

    Info<< "\nStarting time loop\n" << endl;

    while (simple.loop())
    {
        Info<< "Time = " << runTime.timeName() << nl << endl;
        std::fprintf(stderr, "TRACE:loop 1 - after Time=\n"); std::fflush(stderr);

        // Do any mesh changes
        mesh.controlledUpdate();
        std::fprintf(stderr, "TRACE:loop 2 - after controlledUpdate\n"); std::fflush(stderr);

        if (mesh.changing())
        {
            MRF.update();
        }
        std::fprintf(stderr, "TRACE:loop 3 - before UEqn\n"); std::fflush(stderr);

        // --- Pressure-velocity SIMPLE corrector
        {
            #include "UEqn.H"
            std::fprintf(stderr, "TRACE:loop 4 - after UEqn, before pEqn\n"); std::fflush(stderr);
            #include "pEqn.H"
            std::fprintf(stderr, "TRACE:loop 5 - after pEqn\n"); std::fflush(stderr);
        }

        std::fprintf(stderr, "TRACE:loop 6 - before laminarTransport.correct\n"); std::fflush(stderr);
        laminarTransport.correct();
        std::fprintf(stderr, "TRACE:loop 7 - before turbulence->correct\n"); std::fflush(stderr);
        turbulence->correct();
        std::fprintf(stderr, "TRACE:loop 8 - before write\n"); std::fflush(stderr);

        runTime.write();

        #include "poolOccupancy.H"
        runTime.printExecutionTime(Info);
    }

    #include "poolMaxOccupancy.H"

    Info<< "End\n" << endl;

    return 0;
}


// ************************************************************************* //
