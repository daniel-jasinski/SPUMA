/*---------------------------------------------------------------------------*\
  =========                 |
  \\      /  F ield         | OpenFOAM: The Open Source CFD Toolbox
   \\    /   O peration     |
    \\  /    A nd           | www.openfoam.com
     \\/     M anipulation  |
-------------------------------------------------------------------------------
    Copyright (C) 2011-2016 OpenFOAM Foundation
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

Global
    createPhi

Description
    Creates and initialises the relative face-flux field phi.

\*---------------------------------------------------------------------------*/

// * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * //

Info<< "Reading/calculating face flux field phi\n" << endl;
std::fprintf(stderr, "TRACE:phi 1a - about to lookup scheme\n"); std::fflush(stderr);
auto fluxScheme = fvc::scheme<vector>(mesh, "flux(U)");
std::fprintf(stderr, "TRACE:phi 1b - scheme found, getting Sf\n"); std::fflush(stderr);
const surfaceVectorField& phiSf = mesh.Sf();
std::fprintf(stderr, "TRACE:phi 1c - Sf size=%d, U size=%d\n", int(phiSf.primitiveField().size()), int(U.primitiveField().size())); std::fflush(stderr);
std::fprintf(stderr, "TRACE:phi 1d - Sf data=%p, U data=%p\n", (const void*)phiSf.primitiveField().cdata(), (const void*)U.primitiveField().cdata()); std::fflush(stderr);
// Step-by-step surfaceScalarField construction to find crash
std::fprintf(stderr, "TRACE:phi 1e - test Field<scalar> alloc\n"); std::fflush(stderr);
{
    Field<scalar> testF(24170);
    std::fprintf(stderr, "TRACE:phi 1e2 - Field OK, size=%d\n", int(testF.size())); std::fflush(stderr);
}
std::fprintf(stderr, "TRACE:phi 1f - test IOobject construction\n"); std::fflush(stderr);
{
    IOobject testIO("testW", mesh.pointsInstance(), mesh, IOobject::NO_READ, IOobject::NO_WRITE, IOobject::NO_REGISTER);
    std::fprintf(stderr, "TRACE:phi 1f2 - IOobject OK, name=%s\n", testIO.name().c_str()); std::fflush(stderr);
}
std::fprintf(stderr, "TRACE:phi 1g - test DimensionedField<scalar,surfaceMesh>\n"); std::fflush(stderr);
{
    DimensionedField<scalar, surfaceMesh> testDF
    (
        IOobject("testDF", mesh.pointsInstance(), mesh, IOobject::NO_READ, IOobject::NO_WRITE, IOobject::NO_REGISTER),
        mesh,
        dimless,
        false
    );
    std::fprintf(stderr, "TRACE:phi 1g2 - DimField OK, size=%d\n", int(testDF.size())); std::fflush(stderr);
}
std::fprintf(stderr, "TRACE:phi 1h - test full surfaceScalarField\n"); std::fflush(stderr);
{
    surfaceScalarField testSF
    (
        IOobject("testSF", mesh.pointsInstance(), mesh, IOobject::NO_READ, IOobject::NO_WRITE, IOobject::NO_REGISTER),
        mesh,
        dimless
    );
    std::fprintf(stderr, "TRACE:phi 1h2 - surfaceScalarField OK, size=%d\n", int(testSF.primitiveField().size())); std::fflush(stderr);
}
std::fprintf(stderr, "TRACE:phi 1i - test mesh.weights()\n"); std::fflush(stderr);
{
    const surfaceScalarField& mw = mesh.weights();
    std::fprintf(stderr, "TRACE:phi 1i2 - mesh.weights() OK, size=%d\n", int(mw.primitiveField().size())); std::fflush(stderr);
}
std::fprintf(stderr, "TRACE:phi 1j - test scheme.weights(U)\n"); std::fflush(stderr);
{
    auto tw = fluxScheme().weights(U);
    std::fprintf(stderr, "TRACE:phi 1j2 - scheme.weights OK, size=%d\n", int(tw().primitiveField().size())); std::fflush(stderr);
}
std::fprintf(stderr, "TRACE:phi 1k - about to dotInterpolate\n"); std::fflush(stderr);
auto phiTmp = fluxScheme().dotInterpolate(phiSf, U);
std::fprintf(stderr, "TRACE:phi 2 - after fvc::flux(U), before surfaceScalarField ctor\n"); std::fflush(stderr);
surfaceScalarField phi
(
    IOobject
    (
        "phi",
        runTime.timeName(),
        mesh,
        IOobject::READ_IF_PRESENT,
        IOobject::AUTO_WRITE
    ),
    phiTmp
);
std::fprintf(stderr, "TRACE:phi 3 - after surfaceScalarField ctor\n"); std::fflush(stderr);


// ************************************************************************* //
