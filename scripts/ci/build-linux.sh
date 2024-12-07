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
python_profile="$PWD/profile-python.local.conan"
cat <<- EOF > "$python_profile"
include($PWD/scripts/ci/profiles/cross-linux.profile)
include($PWD/scripts/ci/profiles/$triple.profile)
include($PWD/scripts/ci/profiles/alpaqa-python-linux.profile)
[conf]
tools.build:exelinkflags+=["-static-libgcc"]
tools.build:sharedlinkflags+=["-static-libgcc"]
[options]
alpaqa/*:with_conan_python=True
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
cmake.options.CMAKE_C_COMPILER_LAUNCHER=sccache
cmake.options.CMAKE_CXX_COMPILER_LAUNCHER=sccache
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
