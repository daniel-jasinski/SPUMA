    // Energy equation

    fvScalarMatrix TEqn
    (
        fvm::div(phi, T)
      + fvc::div(phi, volScalarField("K", 0.5*magSqr(U)/(thermo.Cp())))
      - fvm::laplacian(thermalTurbulence->alphaEff()/rho, T)
      == 
        (1.0/thermo.Cp())*(U&g) +
        fvOptions(T)
    );

    TEqn.relax();

    TEqn.solve();

