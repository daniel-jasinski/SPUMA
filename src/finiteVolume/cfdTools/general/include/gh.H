    Info<< "Calculating field g.h\n" << endl;
    dimensionedScalar ghRef
    (
        mag(g.value()) > SMALL
      ? g & (cmptMag(g.value())/mag(g.value()))*hRef
      : dimensionedScalar("ghRef", g.dimensions()*dimLength, 0)
    );
    const int oldLocal = volVectorField::Boundary::localConsistencyRef();
    volVectorField::Boundary::localConsistencyRef() = 0;

    volScalarField gh("gh", (g & mesh.C()) - ghRef);
    surfaceScalarField ghf("ghf", (g & mesh.Cf()) - ghRef);

    volVectorField::Boundary::localConsistencyRef() = oldLocal;
