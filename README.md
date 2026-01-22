## About SPUMA

SPUMA[^1] (**S**imulation **P**rocessing in **U**nified **M**emory
**A**ccelerators) is a CFD software released by Cineca based on OpenFOAM®
technology[^2].
It implements a full GPU porting of OpenFOAM targeting NVIDIA and AMD GPUs.

## License

SPUMA is free software: you can redistribute it and/or modify it under the terms of
the GNU General Public License as published by the Free Software Foundation, either
version 3 of the License, or (at your option) any later version.  See the file
COPYING in this directory or
[http://www.gnu.org/licenses/](http://www.gnu.org/licenses), for a description of
the GNU General Public License terms under which you may redistribute files.

## Versioning

SPUMA versioning follows the format:
```
M.m-vFFFF
```
Where `M` represents the major version of the software, `m` the minor version and
`vFFFF` indicates the OpenCFD® release of OpenFOAM it is based on.  For example,
SPUMA version `0.1-v2412` is the first minor version of major version 0, based on
the 2412 release of OpenFOAM by OpenCFD®.

## Using SPUMA

Usage of SPUMA is identical to that of the OpenFOAM release it is based on, with
the exception that executables accept two additional command line arguments:
`-pool` and `-poolSize`. These are used to specify which type of memory pool to use
and its size (if applicable):
```
<executable>Foam -pool <typeOfMemoryPool> [-poolSize <sizeInGigaBytes>]
```
Available memory pool implementations are:

* `dummyMemoryPool` : default option. Allocates all data objects individually (same
as standard OpenFOAM). This option should not be used when running on GPUs as it
affects performance negatively.
* `fixedSizeMemoryPool` : allocates a fixed size block of memory and places data
objects inside it.
* `umpireMemoryPool` : uses the Umpire[^3] library for memory management
(see [below](#compile-with-umpire-support) for more information on compiling SPUMA
with Umpire support)

`fixedSizeMemoryPool` requires the specification of the memory block's size. Care
should be taken to ensure that enough memory is allocated for the intended
application.

For all other aspects of SPUMA utilization you can refer to the
[official OpenFOAM documentation](https://www.openfoam.com/documentation/overview).

## Compiling SPUMA

SPUMA can be compiled to target three different hardware configurations:
* CPU only
* Nvidia GPUs (CUDA backend)
* AMD GPUs (HIP backend)

The procedure to compile the CPU only version of SPUMA is the same as that for the
OpenFOAM release it is based on. To compile SPUMA for GPUs, some additional steps
are required, as presented below. For all other requirements and procedures to
compile SPUMA you can refer to the
[official OpenFOAM build guide](https://develop.openfoam.com/Development/openfoam/blob/develop/doc/Build.md).

#### Nvidia CUDA version

Requires the `Nvidia HPC` (version >= 22.3) and `Nvidia CUDA` (version >= 11.6)
compilers. To build SPUMA with the Nvidia compiler, change the following variables
in the `etc/bashrc` configuration file to:
```
WM_COMPILER=Nvidia
FOAM_SIGFPE=false
```
Then run `source etc/bashrc` from the main SPUMA folder. To enable the CUDA GPU
backend, set the following environment variables as:
```
export have_cuda=true
export NVARCH=<compute capability>
```
`NVARCH` is set to `80` by default, which is the maximum compute capability of
Nvidia A100 GPUs. You should check your target hardware's maximum compute
capability and set the `NVARCH` variable to that value. Lower values will result
in a working build not optimized for your target hardware. Higher values will
result in a build completely incompatible with your target hardware.

Finally, run the `./Allwmake` script from the main folder to compile SPUMA.

More information on compiling SPUMA for Nvidia GPUs can be found at
[SPUMA/Wiki/How-to-build](https://gitlab.hpc.cineca.it/exafoam/spuma/-/wikis/How-to-build).

#### AMD HIP version

Requires the `AMD ROCm` (version >= 6.0.3) compiler. To build SPUMA with the ROCm
compiler, change the following variables in the `etc/bashrc` configuration file to:
```
WM_COMPILER=Amdclang
FOAM_SIGFPE=false
```
Then run `source etc/bashrc` from the main SPUMA folder. To enable the HIP GPU
backend, set the following environment variables as:
```
export have_hip=true
export HSA_XNACK=1
export AMDARCH=<target architecture>
```
`AMDARCH` is set to `gfx90a` by default, which is the architecture of the MI200
series of AMD GPUs. Check your target hardware architecture and set `AMDARCH` to
its corresponding value. Setting `AMDARCH=native` should detect the target hardware
automatically but this may not always work.

Finally, run the `./Allwmake` script from the main folder to compile SPUMA.

More information on compiling SPUMA for AMD GPUs can be found at
[SPUMA/Wiki/How-to-build](https://gitlab.hpc.cineca.it/exafoam/spuma/-/wikis/How-to-build).

#### Compile with Umpire support

To compile SPUMA with support enabled for the Umpire library, you need to set the
following environment variables as:
```
export have_umpire=true
export UMPIRE_HOME=<umpire library installation directory>
```
Note that you need a distribution of Umpire with CUDA support enabled or HIP support
enabled to run SPUMA on Nvidia or AMD GPUs respectively.

More information on how to compile the Umpire library with GPU support on specific
systems can be found at:
[SPUMA/Wiki/Umpire](https://gitlab.hpc.cineca.it/exafoam/spuma/-/wikis/Umpire)

#### Multi-GPU

SPUMA can run on multiple GPUs. To avoid costly memory transfers, a GPU-aware
distribution of MPI should be used when building and running the software.
Additional compiler flags may be required when compiling the code to enable
inter-GPU communication. Refer to your cluster's documentation for additional
information.

## Tutorials

We created some tutorials already set up to be run on GPUs using SPUMA.
The complete list can be found at:
[SPUMA/Wiki/Tutorials](https://gitlab.hpc.cineca.it/exafoam/spuma/-/wikis/Tutorials)

## GPU Support

**SPUMA is a work in progress**.  Not all solvers and functionalities of OpenFOAM
have been ported to GPU. Thanks to the unified memory approach, non-ported
functionalities should run correctly on CPU over the course of a GPU run, at the
cost of additional memory transfers, however, **we cannot guarantee full
interoperability of non-ported functionalities with standard OpenFOAM**.
A list of functionalities and solvers currently supported on GPU can be found
at [SPUMA/Wiki/GPU support](https://gitlab.hpc.cineca.it/exafoam/spuma/-/wikis/GPU-support).

[^1]: This offering is not approved or endorsed by OpenCFD Limited, producer and
distributor of the OpenFOAM software via www.openfoam.com, and owner of the
OPENFOAM® and OpenCFD® trade marks.

[^2]: OPENFOAM® is a registered trade mark of OpenCFD Limited, producer and
distributor of the OpenFOAM software via www.openfoam.com.

[^3]: The Umpire library is released by the Lawrence Livermore National Laboratory
at [github.com/LLNL/Umpire](https://github.com/LLNL/Umpire).
