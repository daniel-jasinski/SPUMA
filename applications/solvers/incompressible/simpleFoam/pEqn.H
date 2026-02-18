{
    std::fprintf(stderr, "TRACE:pEqn 1 - rAU\n"); std::fflush(stderr);
    volScalarField rAU(1.0/UEqn.A());
    std::fprintf(stderr, "TRACE:pEqn 2 - HbyA\n"); std::fflush(stderr);
    volVectorField HbyA(constrainHbyA(rAU*UEqn.H(), U, p));
    std::fprintf(stderr, "TRACE:pEqn 3 - phiHbyA\n"); std::fflush(stderr);
    surfaceScalarField phiHbyA("phiHbyA", fvc::flux(HbyA));
    MRF.makeRelative(phiHbyA);
    adjustPhi(phiHbyA, U, p);

    tmp<volScalarField> rAtU(rAU);

    std::fprintf(stderr, "TRACE:pEqn 4 - consistent check\n"); std::fflush(stderr);
    if (simple.consistent())
    {
        std::fprintf(stderr, "TRACE:pEqn 4a - consistent rAtU\n"); std::fflush(stderr);
        rAtU = 1.0/(1.0/rAU - UEqn.H1());
        std::fprintf(stderr, "TRACE:pEqn 4b - phiHbyA correction\n"); std::fflush(stderr);
        phiHbyA +=
            fvc::interpolate(rAtU() - rAU)*fvc::snGrad(p)*mesh.magSf();
        std::fprintf(stderr, "TRACE:pEqn 4c - HbyA correction\n"); std::fflush(stderr);
        HbyA -= (rAU - rAtU())*fvc::grad(p);
    }

    std::fprintf(stderr, "TRACE:pEqn 5 - tUEqn.clear\n"); std::fflush(stderr);
    tUEqn.clear();

    // Update the pressure BCs to ensure flux consistency
    std::fprintf(stderr, "TRACE:pEqn 6 - constrainPressure\n"); std::fflush(stderr);
    constrainPressure(p, U, phiHbyA, rAtU(), MRF);

    // Non-orthogonal pressure corrector loop
    std::fprintf(stderr, "TRACE:pEqn 7 - non-ortho loop start\n"); std::fflush(stderr);
    while (simple.correctNonOrthogonal())
    {
        std::fprintf(stderr, "TRACE:pEqn 7a - building pEqn matrix\n"); std::fflush(stderr);
        fvScalarMatrix pEqn
        (
            fvm::laplacian(rAtU(), p) == fvc::div(phiHbyA)
        );

        std::fprintf(stderr, "TRACE:pEqn 7b - setReference\n"); std::fflush(stderr);
        pEqn.setReference(pRefCell, pRefValue);

        std::fprintf(stderr, "TRACE:pEqn 7c - solve\n"); std::fflush(stderr);
        pEqn.solve();
        std::fprintf(stderr, "TRACE:pEqn 7d - solve done\n"); std::fflush(stderr);

        if (simple.finalNonOrthogonalIter())
        {
            std::fprintf(stderr, "TRACE:pEqn 7e - final iter flux\n"); std::fflush(stderr);
            phi = phiHbyA - pEqn.flux();
            std::fprintf(stderr, "TRACE:pEqn 7f - flux done\n"); std::fflush(stderr);
        }
    }

    std::fprintf(stderr, "TRACE:pEqn 8 - continuityErrs\n"); std::fflush(stderr);
    {
        std::fprintf(stderr, "TRACE:contErr 1 - fvc::div(phi)\n"); std::fflush(stderr);
        volScalarField contErr(fvc::div(phi));
        std::fprintf(stderr, "TRACE:contErr 2a - deltaTValue\n"); std::fflush(stderr);
        scalar dtVal = runTime.deltaTValue();
        std::fprintf(stderr, "TRACE:contErr 2b - mag(contErr)\n"); std::fflush(stderr);
        tmp<volScalarField> tMagContErr = mag(contErr);
        std::fprintf(stderr, "TRACE:contErr 2c - deref tmp\n"); std::fflush(stderr);
        const volScalarField& magContErr = tMagContErr();
        std::fprintf(stderr, "TRACE:contErr 2d - mesh.V()\n"); std::fflush(stderr);
        const DimensionedField<scalar, volMesh>& V = mesh.V();
        std::fprintf(stderr, "TRACE:contErr 2e - weightedAverage\n"); std::fflush(stderr);
        dimensioned<scalar> wavg = magContErr.weightedAverage(V);
        std::fprintf(stderr, "TRACE:contErr 2f - value()\n"); std::fflush(stderr);
        scalar sumLocalContErr = dtVal * wavg.value();
        std::fprintf(stderr, "TRACE:contErr 3 - globalContErr\n"); std::fflush(stderr);
        scalar globalContErr = runTime.deltaTValue()*
            contErr.weightedAverage(mesh.V()).value();
        std::fprintf(stderr, "TRACE:contErr 4 - cumulative\n"); std::fflush(stderr);
        cumulativeContErr += globalContErr;
        std::fprintf(stderr, "TRACE:contErr 5 - Info\n"); std::fflush(stderr);
        Info<< "time step continuity errors : sum local = " << sumLocalContErr
            << ", global = " << globalContErr
            << ", cumulative = " << cumulativeContErr
            << endl;
        std::fprintf(stderr, "TRACE:contErr 6 - done\n"); std::fflush(stderr);
    }

    // Explicitly relax pressure for momentum corrector
    std::fprintf(stderr, "TRACE:pEqn 9 - p.relax\n"); std::fflush(stderr);
    p.relax();

    // Momentum corrector
    std::fprintf(stderr, "TRACE:pEqn 10 - momentum corrector\n"); std::fflush(stderr);
    U = HbyA - rAtU()*fvc::grad(p);
    std::fprintf(stderr, "TRACE:pEqn 11 - correctBoundaryConditions\n"); std::fflush(stderr);
    U.correctBoundaryConditions();
    std::fprintf(stderr, "TRACE:pEqn 12 - fvOptions.correct\n"); std::fflush(stderr);
    fvOptions.correct(U);
    std::fprintf(stderr, "TRACE:pEqn 13 - done\n"); std::fflush(stderr);
}
