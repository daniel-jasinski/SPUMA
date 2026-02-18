    // Momentum predictor
    std::fprintf(stderr, "TRACE:UEqn 1 - correctBoundaryVelocity\n"); std::fflush(stderr);
    MRF.correctBoundaryVelocity(U);
    std::fprintf(stderr, "TRACE:UEqn 2 - building UEqn matrix\n"); std::fflush(stderr);

    tmp<fvVectorMatrix> tUEqn
    (
        fvm::div(phi, U)
      + MRF.DDt(U)
      + turbulence->divDevReff(U)
     ==
        fvOptions(U)
    );
    std::fprintf(stderr, "TRACE:UEqn 3 - UEqn built, getting ref\n"); std::fflush(stderr);
    fvVectorMatrix& UEqn = tUEqn.ref();

    std::fprintf(stderr, "TRACE:UEqn 4 - relax\n"); std::fflush(stderr);
    UEqn.relax();
    std::fprintf(stderr, "TRACE:UEqn 5 - constrain\n"); std::fflush(stderr);

    fvOptions.constrain(UEqn);
    std::fprintf(stderr, "TRACE:UEqn 6 - momentumPredictor check\n"); std::fflush(stderr);

    if (simple.momentumPredictor())
    {
        std::fprintf(stderr, "TRACE:UEqn 7 - computing fvc::grad(p)\n"); std::fflush(stderr);
        auto gradP = fvc::grad(p);
        std::fprintf(stderr, "TRACE:UEqn 8 - about to solve\n"); std::fflush(stderr);

        // Break solve into steps to find crash
        {
            fvVectorMatrix eqn(UEqn == -gradP);
            std::fprintf(stderr, "TRACE:UEqn 8a - eqn built\n"); std::fflush(stderr);
            const dictionary& solverControls = eqn.solverDict();
            std::fprintf(stderr, "TRACE:UEqn 8b - solverDict found, solver=%s\n",
                solverControls.getOrDefault<word>("solver", word("none")).c_str()); std::fflush(stderr);
            eqn.solve(solverControls);
            std::fprintf(stderr, "TRACE:UEqn 9 - solve done\n"); std::fflush(stderr);
        }

        fvOptions.correct(U);
        std::fprintf(stderr, "TRACE:UEqn 10 - correct done\n"); std::fflush(stderr);
    }
