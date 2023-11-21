# alpaqa CMake example: Solver usage

This is a CMake example project that makes use of the alpaqa solvers. It
demonstrates how to easily build and install all dependencies using Conan, and
how you can add the alpaqa libraries to your CMake configuration.

The project itself is very simple:
1. A problem class is defined in `problem.{hpp,cpp}`, which represents a
   standard quadratic program (QP) formulation.
2. The matrices and vectors comprising the QP are loaded from the CSV files in
   the `data` directory.
3. The main function defined in `alpaqa-qp-solver.cpp` includes one of the
   alpaqa solvers, configures some parameters, and then applies it to the QP.

## Before you start

You'll need a C++ compiler (e.g. GCC, Clang, MSVC), CMake and Conan.

The latter two can be installed using Pip:

```sh
pip install -U conan cmake
```

## Instructions

Build and package alpaqa:

```sh
git clone https://github.com/kul-optec/alpaqa
conan create alpaqa
```

Install the dependencies for the example project:

```sh
cd alpaqa/examples/CMake/Solver
conan install .
```

Configure and build the example project:

```sh
# Linux/macOS (single-configuration generator)
cmake --preset conan-release
cmake --build --preset conan-release -j
```
```sh
# Windows (multi-configuration generator)
cmake --preset conan-default
cmake --build --preset conan-release -j
```

Run the example:

```sh
./build/Release/alpaqa-qp-solver data
```

---

## Advanced

### Shared libraries

To link to the alpaqa shared libraries instead of statically, use the following
Conan options:

```sh
conan create alpaqa -o shared=True
cd alpaqa/examples/CMake/Solver
conan install . --options='alpaqa/*:shared=True'
```

### ABI issues

The advantage of Conan is that it will consistently use the same options and the
same versions of dependencies (e.g. Eigen) when building alpaqa itself and when
building your project.

If you decide not to use Conan, it is crucial that you build both alpaqa and
your project with the same version of Eigen, and with the same compiler flags.
If you don't, you will likely end up with ABI incompatibilities, and your code
will not work.

For example, changing the architecture flags (e.g. `-march=skylake`) can change
the Eigen ABI, because different CPUs with different vector extensions require
different alignments.

### CasADi

If you need optional alpaqa features, e.g. the CasADi interface, you'll have to
specify them explicitly.

For example, to enable CasADi, first build CasADi itself, and then rebuild
alpaqa with CasADi support enabled:
```sh
conan create alpaqa/scripts/recipes/casadi
conan create alpaqa -o with_casadi=True
```

Then add the following options at the bottom of your project's `conanfile.txt`:
```conanfile
[options]
alpaqa*:with_casadi=True
```

In your project's `CMakeLists.txt`, enable the optional CasADi component:

```cmake
# Locate the alpaqa package
find_package(alpaqa 1.0.0 REQUIRED COMPONENTS CasADi)
# Alternatively, use OPTIONAL_COMPONENTS if CasADi is optional
```

You can now use the alpaqa CasADi loader target:
```cmake
target_link_libraries(alpaqa-qp-solver PRIVATE alpaqa::casadi-loader)
```

Delete your project's build folder, then reinstall and reconfigure:
```sh
cd alpaqa/examples/CMake/Solver
rm -rf build # or rm -r -Fo build on Windows
conan install .
cmake --preset conan-release # or conan-default on Windows
cmake --build --preset conan-release -j
```

### Disabling tests

By default, the `conan create` command will also build and run alpaqa's unit
tests. To speed up the process, you may decide to disable them:

```sh
conan create alpaqa -c tools.build:skip_test=True
```
