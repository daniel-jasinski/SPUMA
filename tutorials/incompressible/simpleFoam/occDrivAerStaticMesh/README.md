# Open-closed cooling DrivAer variant with Static Mesh

This test case is based on the one available at the <a rel="hpcCommitteeRepo" href="https://develop.openfoam.com/committees/hpc/-/tree/develop/incompressible/simpleFoam/occDrivAerStaticMesh">OpenFOAM HPC committee repository</a>.

## Obtaining mesh files
The polyMesh files can be obtained from the following links:
* Coarse - <a rel="coarse" href="https://zenodo.org/records/15012221/files/polyMesh_65M.tar.gz?download=1">polyMesh_65M.tar.gz</a>
* Medium - <a rel="medium" href="https://zenodo.org/records/15012221/files/polyMesh_110M.tar.gz?download=1">polyMesh_110M.tar.gz</a>
* Fine - <a rel="fine" href="https://zenodo.org/records/15012221/files/polyMesh_236M.tar.gz?download=1">polyMesh_236M.tar.gz</a>

There is no requirement to download all three sets of mesh files

After downloading, copy the tar file to the `constant/` folder, and untar with `tar -zxvf polyMesh_65M.tar.gz`.

## Using SPUMA:

The case is set for the coarse (65M) mesh. For larger meshes, it may be necessary to increase the `poolSize` variable in `Allrun`. A value of least 62 GBs is required for running `decomposePar` on the fine (236M) mesh. If your device's memory is not sufficient, run the pre-processing steps with standard OpenFOAM and use SPUMA for the computation.

#### Advanced:

* *SPUMA* includes a coupled linear solver for the velocity. A sample setup for the coupled solver is contained in `fvSolution.coupledU`.

* Fusing patches improves performance. To use fused patches, copy the corresponding boundary file from folder `fusedPatches/` to `constant/polyMesh/` and use the `system/include/caseDefinition.fusedPatches` setup file.

* To use the **Scotch** decomposition method you need to compile *SPUMA* with third party libraries. Refer to the <a rel="thirdPartyRepo" href="https://develop.openfoam.com/Development/ThirdParty-common/blob/develop/BUILD.md">official OpenFOAM documentation</a> for instructions. Change the `decompositionMethod` field to `scotch` in `system/include/caseDefinition` to use the library.

* To use the **AmgX** library, you need to download and compile the <a rel="foamExternalSolvers" href="https://gitlab.hpc.cineca.it/exafoam/foamExternalSolvers">foamExternalSolvers</a> library and add `"$FOAM_USER_LIBBIN/libAmgX4Foam.so"` to the `libs` list in `system/controlDict`. The **AmgX** library is <u>not</u> included in *foamExternalSolvers* but can be downloaded from the <a rel="AmgX" href = "https://github.com/NVIDIA/AMGX">official Nvidia repository</a>. A sample setup for using **AmgX** to solve the pressure equation is contained in `fvSolution.AmgX`.
