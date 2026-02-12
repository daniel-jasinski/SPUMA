std::fprintf(stderr, "TRACE:sf 1 - before IOobject p\n"); std::fflush(stderr);
Info<< "Reading field p\n" << endl;
std::fprintf(stderr, "TRACE:sf 2 - before volScalarField p ctor\n"); std::fflush(stderr);
volScalarField p
(
    IOobject
    (
        "p",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);
std::fprintf(stderr, "TRACE:sf 3 - after volScalarField p\n"); std::fflush(stderr);

Info<< "Reading field U\n" << endl;
std::fprintf(stderr, "TRACE:sf 4 - before volVectorField U ctor\n"); std::fflush(stderr);
volVectorField U
(
    IOobject
    (
        "U",
        runTime.timeName(),
        mesh,
        IOobject::MUST_READ,
        IOobject::AUTO_WRITE
    ),
    mesh
);
std::fprintf(stderr, "TRACE:sf 5 - after volVectorField U\n"); std::fflush(stderr);

#include "createPhi.H"
std::fprintf(stderr, "TRACE:sf 6 - after createPhi\n"); std::fflush(stderr);

label pRefCell = 0;
scalar pRefValue = 0.0;
setRefCell(p, simple.dict(), pRefCell, pRefValue);
mesh.setFluxRequired(p.name());
std::fprintf(stderr, "TRACE:sf 7 - after setRefCell\n"); std::fflush(stderr);

singlePhaseTransportModel laminarTransport(U, phi);
std::fprintf(stderr, "TRACE:sf 8 - after laminarTransport\n"); std::fflush(stderr);

autoPtr<incompressible::turbulenceModel> turbulence
(
    incompressible::turbulenceModel::New(U, phi, laminarTransport)
);
std::fprintf(stderr, "TRACE:sf 9 - after turbulence\n"); std::fflush(stderr);

#include "createMRF.H"
std::fprintf(stderr, "TRACE:sf 10 - after MRF\n"); std::fflush(stderr);
#include "createFvOptions.H"
std::fprintf(stderr, "TRACE:sf 11 - after fvOptions\n"); std::fflush(stderr);
