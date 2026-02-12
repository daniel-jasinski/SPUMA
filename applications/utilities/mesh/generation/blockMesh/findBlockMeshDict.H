/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2021-2022 OpenCFD Ltd.
-------------------------------------------------------------------------------
License
    This file is part of OpenFOAM, distributed under GPL-3.0-or-later.

Description
    Search for the appropriate blockMeshDict dictionary...

\*---------------------------------------------------------------------------*/

// Search for the appropriate blockMesh dictionary....
std::fprintf(stderr, "TRACE: findDict 1 - creating dictName\n"); std::fflush(stderr);
const word dictName("blockMeshDict");
std::fprintf(stderr, "TRACE: findDict 2 - dictName created\n"); std::fflush(stderr);

autoPtr<IOdictionary> meshDictPtr;

{
    fileName dictPath;
    std::fprintf(stderr, "TRACE: findDict 3 - before regionName\n"); std::fflush(stderr);
    const word& regionDir = polyMesh::regionName(regionName);
    std::fprintf(stderr, "TRACE: findDict 4 - after regionName\n"); std::fflush(stderr);

    if (args.readIfPresent("dict", dictPath))
    {
        // Dictionary specified on the command-line ...

        if (isDir(dictPath))
        {
            dictPath /= dictName;
        }
    }
    else if
    (
        exists
        (
            runTime.path()/runTime.constant()
          / polyMesh::meshDir(regionName)/dictName
        )
    )
    {
        // Dictionary present in constant polyMesh directory (old-style)

        dictPath =
        (
            runTime.constant()
          / polyMesh::meshDir(regionName)/dictName
        );


        // Warn that constant/polyMesh/blockMeshDict was used
        // instead of system/blockMeshDict
        WarningIn(args.executable())
            << "Using the old blockMeshDict location: "
            << dictPath << nl
            << "    instead of the default location:  "
            << runTime.system()/regionDir/dictName << nl
            << endl;
    }
    else
    {
        // Assume dictionary is to be found in the system directory

        dictPath = runTime.system()/regionDir/dictName;
    }
    std::fprintf(stderr, "TRACE: findDict 5 - dictPath='%s'\n", dictPath.c_str()); std::fflush(stderr);

    IOobject meshDictIO
    (
        dictPath,
        runTime,
        IOobject::MUST_READ,
        IOobject::NO_WRITE,
        IOobject::NO_REGISTER
    );
    std::fprintf(stderr, "TRACE: findDict 6 - IOobject created\n"); std::fflush(stderr);
    std::fprintf(stderr, "TRACE: findDict 6a - IOdictionary::typeName at %p\n", (const void*)&IOdictionary::typeName); std::fflush(stderr);
    std::fprintf(stderr, "TRACE: findDict 6b - about to call typeHeaderOk\n"); std::fflush(stderr);

    if (!meshDictIO.typeHeaderOk<IOdictionary>(true))
    {
        FatalErrorInFunction
            << meshDictIO.objectPath() << nl
            << exit(FatalError);
    }
    std::fprintf(stderr, "TRACE: findDict 7 - header OK\n"); std::fflush(stderr);

    Info<< "Creating block mesh from "
        << meshDictIO.objectRelPath() << endl;
    std::fprintf(stderr, "TRACE: findDict 8 - after Info\n"); std::fflush(stderr);

    meshDictPtr = autoPtr<IOdictionary>::New(meshDictIO);
    std::fprintf(stderr, "TRACE: findDict 9 - dict read\n"); std::fflush(stderr);
}

const IOdictionary& meshDict = *meshDictPtr;
std::fprintf(stderr, "TRACE: findDict 10 - dict deref OK\n"); std::fflush(stderr);


// ************************************************************************* //
