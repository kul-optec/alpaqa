# Installation instructions {#installation}

> **Note**  
> This page contains detailed instructions for building and installing all of
> alpaqa from source.
> For the installation of the Python package without building it from source,
> as well as installation instructions for pre-built, released binaries of the
> C++ library and the MATLAB interface, please see [these instructions](../Sphinx/install/installation.html)
> instead.
> For instructions on how to get alpaqa through the [Conan](https://conan.io/)
> package manager, see the @ref cmake_examples.

## Linux

### Tools
First, install some basic tools: C and C++ compilers, Git, and Python
(you'll need the development version to build alpaqa's Python interface, and we
install the `venv` module to create virtual environments).
```sh
sudo apt install g++ gcc git python3-venv python3-dev
```
The alpaqa package requires a relatively recent compiler
(tested using GCC 10-13, Clang (libc++) 16-17, or Clang (libstdc++) 17).

To install GCC 11 on older versions of Ubuntu, you can use
```sh
sudo apt update
sudo apt install software-properties-common
sudo add-apt-repository ppa:ubuntu-toolchain-r/test
sudo apt install gcc-11 g++-11
```
To install the latest version of Clang, you can use the instructions from <https://apt.llvm.org>:
```sh
bash -c "$(wget -O- https://apt.llvm.org/llvm.sh)"
```

### Clone the repository

```sh
git clone https://github.com/kul-optec/alpaqa --branch=develop --single-branch
```

### Create a virtual environment

For convenience, we'll install everything into a Python virtual environment
(including the C++ libraries and dependencies). This allows you to easily
experiment in a sandbox, without requiring root permissions, and without the
risk of messing with system packages.

```sh
cd alpaqa
python3 -m venv .venv
. ./.venv/bin/activate
pip install cmake ninja casadi numpy
```

### Install dependencies

The `scripts` folder contains some Bash scripts to install the necessary
dependencies. Feel free to inspect and modify the installation scripts.
If you already have the dependencies installed globally you can skip these
steps.

```sh
bash ./scripts/install-casadi-static.sh "$VIRTUAL_ENV" Release
bash ./scripts/install-gtest.sh "$VIRTUAL_ENV" Release
bash ./scripts/install-eigen.sh "$VIRTUAL_ENV" Release
```

CasADi is built as a static library because it is later statically linked into
the final alpaqa libraries for better portability, this is especially useful
when creating the Python package. If you need to link against CasADi
dynamically, you can use the `install-casadi.sh` script instead.

### Build and install

The following commands build and install the alpaqa C++ library into the virtual
environment.  
You may want to change the installation prefix, e.g. use `--prefix /usr/local`
for a system-wide install (requires `sudo`), or `--prefix $HOME/.local` to
install it for the current user.

```sh
cmake -S. -Bbuild -G "Ninja Multi-Config"

cmake --build build --config Release -j       # Build in release mode
cmake --build build -t test                   # Run the tests
cmake --install build --prefix "$VIRTUAL_ENV" # Install the release version

cmake --build build --config Debug -j         # Build in debug mode
cmake --build build -t test                   # Run the tests with extra checks
cmake --install build --prefix "$VIRTUAL_ENV" # Install the debug version
```
Installing both the release and debug versions can be very useful for checking
matrix dimension errors and out of bounds accesses during development, and 
switching to an optimized version later.

> **Note**  
> If you changed the installation prefix, and
> unless you installed the package to a system folder like `/usr/local`, you'll
> have to add `~/.local` to the `CMAKE_PREFIX_PATH`, e.g. by adding the
> following to your `~/.profile` file, where `$HOME/.local` was the prefix used
> in the when installing alpaqa earlier:
> ```sh
> export CMAKE_PREFIX_PATH="$HOME/.local:$CMAKE_PREFIX_PATH"
> ```
> Then source it (`. ~/.profile`) or log out and back in again.

## Windows

The instructions for Windows are quite similar to the ones for Linux. To install
the dependencies, you can use the Powershell scripts instead of the Bash scripts:

```ps
./scripts/install-casadi-static.ps1
./scripts/install-gtest.ps1
./scripts/install-eigen.ps1
```

## macOS

The instructions for macOS are the same as the ones for Linux, with the caveat
that the default AppleClang compiler might not yet support the necessary C++20
features used by alpaqa. If this is the case, you can use a mainline Clang
compiler (version 16 or higher), that you install using Homebrew or another
package manager.  
You can select the compiler to use by setting the `CC` and `CXX` environment
variables and reconfiguring the project, for example:
```sh
export CC=clang-16
export CXX=clang++-16
rm build/CMakeCache.txt  # Remove cache to trigger a fresh CMake configuration
```

If your Clang installation is older than version 16, you'll have to disable the
optional OCP component, by using the `-D ALPAQA_WITH_OCP=Off` CMake option.
This should work for Clang 14 and later.

***

# Usage

Once the library is installed, you can use it in your own projects.

For example:

**main.cpp**
```cpp
#include <alpaqa/panoc-alm.hpp>

int main() {
    // Use the solvers as shown in the examples
}
```

**CMakeLists.txt**
```cmake
cmake_minimum_required(VERSION 3.17)
project(Project)

# Find the library you just installed:
find_package(alpaqa REQUIRED)

add_executable(main main.cpp)
# Link your executable with the library:
target_link_libraries(main PRIVATE alpaqa::alpaqa)
```

Different targets are available. Depending on your needs, you might want to
link to:

 - `alpaqa::alpaqa`: the core alpaqa library and solvers
 - `alpaqa::casadi-loader`: provides the `CasADiProblem` class that allows
    the solvers to interface with problems formulated using CasADi
 - `alpaqa::casadi-ocp-loader`: experimental optimal-control specific CasADi
    problem specification
 - `alpaqa::dl-api`: the stand-alone C API for formulating problems that can be
    loaded dynamically by alpaqa (`alpaqa/dl/dl-problem.h`)
 - `alpaqa::dl-loader`: provides the `DLProblem` class to load such problems
 - `alpaqa::cutest-interface`: provides the `CUTEstProblem` class for loading
    problems formulated using SIF/CUTEst
 - `alpaqa::ipopt-adapter`: allows passing any alpaqa problem to the Ipopt
    solver
 - `alpaqa::lbfgsb-adapter`: allows passing any alpaqa problem to the L-BFGS-B
    solver
 - `alpaqa::qpalm-adapter`: allows passing any alpaqa problem to the QPALM
    solver

# Python

After creating the virtual environment and installing the dependencies, you can
install the Python module using:
```sh
pip install .
```
To build the Python package without installing, you can use:
```sh
pip install build
python3 -m build .
```

# Matlab

The previous steps are not required to install the MATLAB/MEX interface. We'll
use [Conan](https://conan.io/) to manage and build the necessary dependencies.

## Linux and macOS

```sh
python3 -m pip install -U conan cmake ninja
conan profile detect --force
conan create scripts/recipes/casadi --build=missing
conan install . \
    --build=missing \
    -c tools.cmake.cmaketoolchain:generator="Ninja Multi-Config" \
    -of build-matlab \
    -o with_matlab=True -o with_json=True -o with_casadi=True
cmake --preset conan-default
cmake --build --preset conan-release -j -t alpaqa_mex
cmake --install build-matlab/build \
    --prefix ~/Documents/MATLAB --component mex_interface
```

## Windows

```sh
python -m pip install -U conan cmake ninja
conan profile detect --force
conan create scripts/recipes/casadi --build=missing
conan install . \
    --build=missing \
    -of build-matlab \
    -o with_matlab=True -o with_json=True -o with_casadi=True
cmake --preset conan-default
cmake --build --preset conan-release -j -t alpaqa_mex
cmake --install build-matlab/build \
    --prefix "$env:USERPROFILE\Documents\MATLAB" --component mex_interface
```

## Uninstall

To uninstall the alpaqa MATLAB/MEX interface, simply remove the `+alpaqa`
directory, e.g. by running the following command in the MATLAB command window:

```
rmdir(fullfile(userpath, '+alpaqa'), 's')
```
