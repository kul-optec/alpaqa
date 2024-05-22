#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..

set -ex

# Activate virtual environment
[ -d ".venv" ] || python3 -m venv .venv
. ./.venv/bin/activate
python_version="$(python --version | cut -d ' ' -f 2)"
python_majmin="${python_version%.*}"

# Select architecture
triple=x86_64-bionic-linux-gnu

# Download compiler
gcc_version=14
tools_dir="$PWD/toolchains"
pfx="$tools_dir/x-tools/$triple"
mkdir -p "$tools_dir/x-tools"
[ -d "$pfx" ] || {
    chmod u+w "$tools_dir/x-tools"
    url=https://github.com/tttapa/toolchains/releases/latest/download
    wget "$url/x-tools-$triple-gcc$gcc_version.tar.xz" -O- | tar xJ -C "$tools_dir"
    chmod u+w "$pfx"
    url=https://github.com/tttapa/python-dev/releases/latest/download
    wget "$url/python-dev-$triple.tar.xz" -O- | tar xJ -C "$tools_dir"
}

# Download dependencies
pip install -U pip build conan
# My own custom recipes for Ipopt, CasADi, patched OpenBLAS, patched Eigen
[ -d "$tools_dir/thirdparty/conan-recipes" ] || {
    git clone https://github.com/tttapa/conan-recipes "$tools_dir/thirdparty/conan-recipes"
    conan remote add tttapa-conan-recipes "$tools_dir/thirdparty/conan-recipes" --force
}
# QPALM QP solver
[ -d "$tools_dir/thirdparty/QPALM" ] || {
    git clone https://github.com/kul-optec/QPALM --branch=1.2.3 "$tools_dir/thirdparty/QPALM"
    conan export "$tools_dir/thirdparty/QPALM"
}

# Create Conan profile
profile="$PWD/profile.local.conan"
cat <<- EOF > "$profile"
include($pfx.python.profile.conan)
[conf]
tools.build.cross_building:can_run=true
tools.cmake.cmaketoolchain:generator=Ninja Multi-Config
tools.build:skip_test=true
[buildenv]
CFLAGS=-march=native -fdiagnostics-color
CXXFLAGS=-march=native -fdiagnostics-color
LDFLAGS+= -static-libgcc -static-libstdc++
EOF

# Build and install alpaqa dependencies
export CTEST_OUTPUT_ON_FAILURE=1
for cfg in Debug RelWithDebInfo; do
    conan install . --build=missing \
        -pr:h "$profile" \
        -o with_ipopt=True -o with_external_casadi=True -o with_qpalm=True \
        -o with_cutest=True -o "openblas/*:target=HASWELL" \
        -s build_type=$cfg
done

# Configure
cmake --preset conan-default -B build \
    -D CMAKE_INSTALL_PREFIX="$PWD/staging" \
    -D CMAKE_C_COMPILER_LAUNCHER="ccache" \
    -D CMAKE_CXX_COMPILER_LAUNCHER="ccache"
# Build
for cfg in Debug RelWithDebInfo; do
    cmake --build build -j --config $cfg
    cmake --install build --config $cfg
    cmake --install build --config $cfg --component debug
done
# Package
pushd build
cpack -G 'TGZ;DEB' -C "RelWithDebInfo;Debug"
popd

# Build Python package
for cfg in Debug Release; do
    conan install . --build=missing \
        -pr:h "$profile" \
        -o with_ipopt=True -o with_external_casadi=True -o with_qpalm=True \
        -o with_cutest=True -o with_python=True -o "openblas/*:target=HASWELL" \
        -s build_type=$cfg
done
config="$triple.py-build-cmake.config.toml"
cat <<- EOF > "$config"
toolchain_file = "build/generators/conan_toolchain.cmake"
[cmake]
config = ["Debug", "Release"]
generator = "Ninja Multi-Config"
preset = "conan-default"
[cmake.options]
USE_GLOBAL_PYBIND11 = "On" # Provided by Conan, not by Pip
CMAKE_C_COMPILER_LAUNCHER = "ccache"
CMAKE_CXX_COMPILER_LAUNCHER = "ccache"
EOF
cross_cfg="$pfx.python$python_majmin.py-build-cmake.cross.toml"
develop=false
if $develop; then
    pip install -e ".[test]" -v \
        --config-settings=--cross="$cross_cfg" \
        --config-settings=--cross="$PWD/$config"
else
    python -m build -w "." -o staging \
        -C--cross="$cross_cfg" \
        -C--cross="$PWD/$config"
    python -m build -w "python/alpaqa-debug" -o staging \
        -C--cross="$cross_cfg" \
        -C--cross="$PWD/$config"
    pip install -f staging --force-reinstall --no-deps \
        "alpaqa==1.0.0a20" "alpaqa-debug==1.0.0a20"
    pip install -f staging \
        "alpaqa[test]==1.0.0a20" "alpaqa-debug==1.0.0a20"
fi
pytest
