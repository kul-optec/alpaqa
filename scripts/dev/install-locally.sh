#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..
export CTEST_OUTPUT_ON_FAILURE=1
export CLICOLOR_FORCE=1

set -ex

build_python=1
build_cpp=1

case $1 in
    "") ;;
    py*) build_cpp=0 ;;
    cpp*) build_python=0 ;;
    *) echo "Invalid argument, use py[-dev] or cpp[-pkg]"; exit 1 ;;
esac
case $1 in
    py-dev) dev_flags=-e ;;
    *) ;;
esac

# Activate virtual environment
[ -d ".venv" ] || python3 -m venv .venv
. ./.venv/bin/activate
python_version="$(python --version | cut -d ' ' -f 2)"
python_majmin="${python_version%.*}"
python_majmin_nodot="${python_majmin//./}"

# Select architecture
triple=x86_64-bionic-linux-gnu
arch=linux/x86-64-v3
plat_tag=manylinux_2_27_x86_64

# Download dependencies
pip install -U pip build conan
# My own custom recipes for Ipopt, CasADi, QPALM, OpenBLAS, guanaqo, etc.
conan remote add alpaqa-conan-recipes "scripts/ci/conan-recipes" --force

# Check Conan settings
if ! grep toolchain-vendor "$(conan config home)/settings_user.yml" >/dev/null; then
    set +x
    echo "Missing custom Conan settings. Please install the configuration (WARNING: destructive)."
    echo "conan config install \"$PWD/scripts/ci/conan-profiles/settings_user.yml\""
    exit 1
fi

# Create Conan profiles
host_profile="$PWD/profile-host.local.conan"
cat <<- EOF > "$host_profile"
include($PWD/scripts/ci/conan-profiles/profiles/toolchain/$triple.profile)
include($PWD/scripts/ci/conan-profiles/profiles/arch/$arch.profile)
include($PWD/scripts/ci/conan-profiles/profiles/gcc-static.profile)
include($PWD/scripts/ci/conan-profiles/profiles/test/only-self.profile)
include($PWD/scripts/ci/conan-profiles/profiles/link/mold.profile)
include($PWD/scripts/ci/conan-profiles/profiles/tools/ninja.profile)
[conf]
tools.build.cross_building:can_run=True
tools.cmake.cmaketoolchain:generator=Ninja Multi-Config
EOF
build_profile="$PWD/profile-build.local.conan"
cat <<- EOF > "$build_profile"
include(default)
[settings]
mold/*:compiler.cppstd=20
EOF

cpp_profile="$PWD/profile-cpp.local.conan"
cat <<- EOF > "$cpp_profile"
include($host_profile)
include($PWD/scripts/ci/options/alpaqa-cpp-linux.profile)
EOF

# Create Conan profile to inject the appropriate Python development files
python_profile="$PWD/conan-python.profile"
cat <<- EOF > "$python_profile"
include($host_profile)
include($PWD/scripts/ci/options/alpaqa-python-linux.profile)
[options]
&:with_conan_python=True
[replace_requires]
tttapa-python-dev/*: tttapa-python-dev/[~$python_majmin, include_prerelease]
EOF

# Create a py-build-cmake configuration file for cross-compilation
pbc_config="$PWD/$triple.py-build-cmake.pbc"
cat <<- EOF > "$pbc_config"
os=linux
implementation=cp
version="$python_majmin_nodot"
abi="cp$python_majmin_nodot"
arch="$plat_tag"
conan.profile_host=["$python_profile"]
conan.profile_build=["$build_profile"]
conan.cmake.args+=["--fresh"]
# conan.cmake.build_args+=["--verbose"]
EOF

# Build C++ packages
if [ $build_cpp -eq 1 ]; then
    for cfg in Debug RelWithDebInfo; do
        conan install . --build=missing \
            -pr:h "$cpp_profile" \
            -pr:b "$build_profile" \
            -s build_type=$cfg
    done

    # Configure
    cmake --preset conan-default -B build \
        -D CMAKE_EXPORT_COMPILE_COMMANDS=On \
        -D CMAKE_INSTALL_PREFIX="$PWD/staging"
    # Build
    for cfg in Debug RelWithDebInfo; do
        cmake --build build -j --config $cfg
        cmake --install build --config $cfg
        cmake --install build --config $cfg --component debug
    done
    # Package
    if [ "${1: -4}" = "-pkg" ]; then
        pushd build
        cpack -G 'TGZ;DEB' -C "RelWithDebInfo;Debug"
        popd
    fi
fi

# Build Python packages
if [ $build_python -eq 1 ]; then
    # Build and install the Python package
    pip install -v $dev_flags ".[test]" -C cross="$pbc_config" # -C o=editable.build_hook=true
    # Run tests
    pytest
fi
