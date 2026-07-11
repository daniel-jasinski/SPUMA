## Getting the code

Links to all code packs are available on https://dl.openfoam.com. For OpenFOAM-v2412:

- https://dl.openfoam.com/source/latest/
- Source: https://dl.openfoam.com/source/v2412/OpenFOAM-v2412.tgz
- ThirdParty: https://dl.openfoam.com/source/v2412/ThirdParty-v2412.tgz

## OpenFOAM&reg; Quick Build Guide

Prior to building, ensure that the [system requirements][link openfoam-require]
are satisfied (including any special [cross-compiling][wiki-cross-compile]
considerations), and source the correct OpenFOAM environment.
For example, for the OpenFOAM-v2412 version:
```
source <installation path>/OpenFOAM-v2412/etc/bashrc
```
e.g. if installed under the `~/openfoam` directory
```
source ~/openfoam/OpenFOAM-v2412/etc/bashrc
```


## Preliminaries

The [third-party][repo third] directory includes a
[readme][link third-readme] and additional information about
[requirements][link third-require] and a more detailed
[build guide][link third-build].

Some known build issues related to specific compiler and VTK library versions
can be found in the [$WM_PROJECT_DIR/doc/BuildIssues.md][link openfoam-issues]
file.

## Native Windows builds

Native Windows builds use the normal OpenFOAM environment and build entry
points. Run them from an MSYS2 shell, select the compiler while sourcing
`etc/bashrc`, and invoke the top-level `Allwmake`. No separate Windows build
script is required.

Common requirements are:

- 64-bit MSYS2 with `bash`, GNU `make`, `sed`, `awk`, `grep` and `flex`;
- Visual Studio 2022 C++ build tools and a Windows 10 or 11 SDK;
- LLVM tools including `clang`, `lld-link`, `llvm-lib` and `llvm-nm`;
- native Windows Python 3 with the `py.exe` launcher;
- the selected accelerator toolkit.

Windows builds currently use the serial (`dummy`) Pstream implementation.
MPI and zlib support are not yet enabled.

### AdaptiveCpp/SYCL

The Windows fixes required by SPUMA have been merged into upstream
AdaptiveCpp. Until they appear in a tagged AdaptiveCpp release, build an
unmodified upstream `develop` revision at commit `33729bc` or newer. This
revision also contains the earlier Win32/OpenMP fix from commit `05194e7`.

Add the resulting AdaptiveCpp `bin` directory to `PATH`, set `ACPP_PATH` to
its installation prefix, and use the standard OpenFOAM setup:

```
export ACPP_PATH=/c/path/to/AdaptiveCpp-install
export PATH="$ACPP_PATH/bin:$PATH"
source etc/bashrc WM_COMPILER=Sycl WM_MPLIB=dummy
export ACPP_TARGETS=generic
./Allwmake -j -s -l
```

The `win64Sycl` rule runs AdaptiveCpp's Python driver with the native Windows
Python launcher. This is required so the driver selects Windows DLLs and paths
instead of Unix deployment artifacts when the build is started from MSYS2.
Set `ACPP_COMMAND` to override the driver command for installations that
provide a native executable or use a different Python launcher.

`ACPP_TARGETS=generic` produces AdaptiveCpp's JIT-capable target. Runtime
device selection is controlled by AdaptiveCpp itself. Use a clean shell when
switching between compiler backends.

### Native CUDA

Set `CUDA_PATH` to the toolkit installation and select the `Cuda` compiler.
`wmake` converts the toolkit path to the short Windows form required by the
MSVC linker. `NVARCH` defaults to `80` when it is not specified.

```
export CUDA_PATH='C:/Program Files/NVIDIA GPU Computing Toolkit/CUDA/v12.x'
export NVARCH=86
source etc/bashrc WM_COMPILER=Cuda WM_MPLIB=dummy
./Allwmake -j -s -l
```

### Native HIP

Set `HIP_PATH` to the ROCm installation and select the `Hip` compiler.
`AMDARCH` defaults to `gfx1030` when it is not specified.

```
export HIP_PATH='C:/Program Files/AMD/ROCm/6.x'
export AMDARCH=gfx1030
source etc/bashrc WM_COMPILER=Hip WM_MPLIB=dummy
./Allwmake -j -s -l
```

The CUDA and HIP backends are experimental. A successful link must not be
treated as runtime validation; validate the resulting solver on the intended
GPU and driver before production use.

If you need to change the default versions for third-party libraries,
or use system libraries for some components, please some additional
information about the [config structure][wiki-config].

## Compile OpenFOAM

The compilation process is self-contained and will compile and install
all OpenFOAM code and dependencies.

- Test the system readiness (optional, not supported for cross-compilation)
```
foamSystemCheck
```
- Change to the main OpenFOAM directory ($WM_PROJECT_DIR).
  If this fails, the environment is not correctly configured.
```
foam
```
- Compile OpenFOAM
```
./Allwmake -s -l
```
- This will start the build process, where the `-s` option is used to reduce
  the level of output, and the `-l` option to record the output in a log file.
  To see all available options use `./Allwmake -help`
- In case you need to stop the compilation, continue later by running
`./Allwmake` again.

## Compile OpenFOAM faster

For faster compilation, users should take advantage of multi-processor
machines when building the code. This is supported directly by `wmake`
and the `Allwmake` scripts. For example,
```
wmake -j               # Use all cores
wmake -j 8             # Use specified number of cores
```
It can also be helpful to use the builtin queuing (the `-queue`
option), which collects subdirectories and dispatches to make in
larger chunks.

The following compilation sequence can be useful:
```
./Allwmake -j -s -q -l
```
This compiles with all cores (-j), reduced output (-s, -silent), with
queuing (-q, -queue) and logs (-l, -log) the output to a file such as
`log.linux64GccDPInt32Opt` for later inspection.

If you use the `-k` option (`-keep-going` = ignore errors) to compile
as much as possible on the first pass, be certain to follow that with
second pass (without the `-k` option) at a later stage to ensure that
you haven't missed any error messages.

## Post-compilation steps

- Open a new shell and source the OpenFOAM environment to see all
  changes (refer to top of page).
- Validate the build (not supported for cross-compilation) by running
```
foamInstallationTest
```
- test any given tutorial case. For example,
```
foamTestTutorial -full incompressible/simpleFoam/pitzDaily
```
- Note: the tutorial test can also be done manually:
```
# Create the user "run" directory:
mkdir -p "$FOAM_RUN"
# Change to the user "run" directory:
run
# Copy tutorial
cp -r "$FOAM_TUTORIALS"/incompressible/simpleFoam/pitzDaily ./
# Run the tutorial
( cd pitzDaily && blockMesh && simpleFoam )
```

### ParaView

OpenFOAM ships with ParaView sources for post-processing OpenFOAM
field results. However, it will [often be sufficient][FAQ ParaView]
to use the paraview version distributed with
the operating system or a [binary package][download ParaView]
and avoid additional compilation complexity.

If you do wish to compile ParaView from source, it is recommended
that you do so ***after*** completing an initial compilation of OpenFOAM.
This gets the process started much more quickly. At a later stage,
the OpenFOAM visualization module can be compiled for a particular
ParaView version _without_ recompiling OpenFOAM itself.

If you decide to compile in two passes, you only need to execute the
top-level `Allwmake` a second time. Do **not** use `wclean` to force a
complete rebuild! This is unnecessary.

More details in the [ThirdParty build guide][link third-build].


------------

<!-- Links -->

[page ParaView]: http://www.paraview.org/
[download ParaView]: https://www.paraview.org/download/
[FAQ ParaView]: https://discourse.paraview.org/t/i-want-to-visualize-my-openfoam-simulation-results-with-paraview-but-im-confused-which-version-should-i-use


<!-- OpenFOAM -->

[repo openfoam]: https://develop.openfoam.com/Development/openfoam/
[repo third]: https://develop.openfoam.com/Development/ThirdParty-common/

[link openfoam-readme]: https://develop.openfoam.com/Development/openfoam/blob/develop/README.md
[link openfoam-issues]: https://develop.openfoam.com/Development/openfoam/blob/develop/doc/BuildIssues.md
[link openfoam-build]: https://develop.openfoam.com/Development/openfoam/blob/develop/doc/Build.md
[link openfoam-require]: https://develop.openfoam.com/Development/openfoam/blob/develop/doc/Requirements.md
[link third-readme]: https://develop.openfoam.com/Development/ThirdParty-common/blob/develop/README.md
[link third-build]: https://develop.openfoam.com/Development/ThirdParty-common/blob/develop/BUILD.md
[link third-require]: https://develop.openfoam.com/Development/ThirdParty-common/blob/develop/Requirements.md

[wiki-cross-compile]: https://develop.openfoam.com/Development/openfoam/-/wikis/building/cross-compile-mingw
[wiki-config]: https://develop.openfoam.com/Development/openfoam/-/wikis/configuring

---
Copyright 2019-2024 OpenCFD Ltd
