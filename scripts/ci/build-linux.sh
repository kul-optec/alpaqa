#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..

set -ex

# Select Python version
build_python_version="$(python3 --version | cut -d' ' -f2)"
python_version="${1:-${build_python_version}}"
python_majmin="$(echo "$python_version" | cut -d'.' -f1,2)"
python_majmin_nodot="${python_majmin//./}"

# Select architecture
triple="${2:-x86_64-bionic-linux-gnu}"
case "$triple" in
    x86_64-centos7-*) plat_tag=manylinux_2_17_x86_64 ;;
    x86_64-bionic-*) plat_tag=manylinux_2_27_x86_64 ;;
    aarch64-rpi3-*) plat_tag=manylinux_2_27_aarch64 ;;
    armv8-rpi3-*) plat_tag=manylinux_2_27_armv7l ;;
    armv7-neon-*) plat_tag=manylinux_2_27_armv7l ;;
    armv6-*) plat_tag=linux_armv6l ;;
    *) echo "Unknown platform ${triple}"; exit 1 ;;
esac

# Package and output directories
pkg_dir="${3:-.}"
out_dir="${4:-dist}"

# Create Conan profiles
host_profile="$PWD/profile-host.local.conan"
cat <<- EOF > "$host_profile"
include($PWD/scripts/ci/$triple.profile)
[settings]
build_type=Release
compiler=gcc
compiler.cppstd=gnu23
compiler.libcxx=libstdc++11
compiler.version=14
[tool_requires]
tttapa-toolchains/1.0.1
[conf]
tools.build:skip_test=true
tools.build:cflags+=["-fdiagnostics-color"]
tools.build:cxxflags+=["-fdiagnostics-color"]
tools.build:exelinkflags+=["-flto=auto", "-static-libstdc++"]
tools.build:sharedlinkflags+=["-flto=auto", "-static-libstdc++"]
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_INIT": "\${CMAKE_SHARED_LINKER_FLAGS_INIT}"}
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT": "\${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT}"}
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT": "\${CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT}"}
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT": "\${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT}"}
tools.cmake.cmaketoolchain:generator=Ninja Multi-Config
[options]
alpaqa/*:with_conan_python=True
EOF

python_profile="$PWD/profile-python.local.conan"
cat <<- EOF > "$python_profile"
include($host_profile)
include($PWD/scripts/ci/alpaqa-python-linux.profile)
[conf]
tools.build:exelinkflags+=["-static-libgcc"]
tools.build:sharedlinkflags+=["-static-libgcc"]
[replace_requires]
tttapa-python-dev/* : tttapa-python-dev/[~$python_majmin]
EOF

pbc_config="$PWD/$triple.py-build-cmake.config.pbc"
cat <<- EOF > "$pbc_config"
os=linux
implementation="cp"
version="$python_majmin_nodot"
abi="cp$python_majmin_nodot"
arch=$plat_tag
cmake.options.CMAKE_C_COMPILER_LAUNCHER=ccache
cmake.options.CMAKE_CXX_COMPILER_LAUNCHER=ccache
EOF

# Install dependencies
rm -rf "$pkg_dir"/build/python-{debug,release}/{generators,CMakeCache.txt}
for cfg in Debug Release; do
    conan install "$pkg_dir" --build=missing \
        -pr:h "$python_profile" \
        -s build_type=$cfg
done

# Build Python packages
python -m build -w "$pkg_dir" -o "$out_dir" \
    -C local="$PWD/scripts/ci/py-build-cmake.toml" \
    -C cross="$pbc_config"
python -m build -w "$pkg_dir/python/alpaqa-debug" -o "$out_dir" \
    -C local="$PWD/scripts/ci/py-build-cmake.toml" \
    -C component="$PWD/scripts/ci/py-build-cmake.component.toml" \
    -C cross="$pbc_config"
