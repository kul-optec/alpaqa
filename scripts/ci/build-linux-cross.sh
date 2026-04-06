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
    x86_64-centos7-*) plat_tag=manylinux_2_17_x86_64; arch=linux/x86-64-v3 ;;
    x86_64-bionic-*) plat_tag=manylinux_2_27_x86_64; arch=linux/x86-64-v3 ;;
    aarch64-rpi3-*) plat_tag=manylinux_2_27_aarch64; arch=linux/cortex-a53 ;;
    armv8-rpi3-*) plat_tag=manylinux_2_27_armv7l; arch=linux/generic ;;
    armv7-neon-*) plat_tag=manylinux_2_27_armv7l; arch=linux/cortex-a9 ;;
    armv6-*) plat_tag=linux_armv6l; arch=linux/generic ;;
    *) echo "Unknown platform ${triple}"; exit 1 ;;
esac

# Package and output directories
pkg_dir="${3:-.}"
out_dir="${4:-dist}"

# Create Conan profile to inject the appropriate Python development files
python_profile="$PWD/conan-python.cross.profile"
profiles="$PWD/scripts/ci/conan-profiles/profiles"
cat << EOF > "$python_profile"
include($PWD/scripts/ci/options/alpaqa-python-linux.profile)
[options]
&:with_conan_python=True
[replace_requires]
tttapa-python-dev/*: tttapa-python-dev/[~$python_majmin, include_prerelease]
EOF

# Create a py-build-cmake configuration file for cross-compilation
pbc_config="$PWD/$triple.py-build-cmake.cross.pbc"
cat << EOF > "$pbc_config"
os=linux
implementation=cp
version="$python_majmin_nodot"
abi="cp$python_majmin_nodot"
arch="$plat_tag"
conan.profile_host=["$profiles/toolchain/$triple.profile"]
conan.profile_host+=["$profiles/arch/$arch.profile"]
conan.profile_host+=["$python_profile"]
conan.profile_host+=["$profiles/gcc-static.profile"]
conan.profile_host+=["$profiles/test/none.profile"]
conan.profile_host+=["$profiles/tools/ninja.profile"]
conan.cmake.args+=["--fresh"]
conan.cmake.build_args+=["--verbose"]
EOF

# Build the Python package
python3 -m pip install -U build
python3 -m build -w "$pkg_dir" -o "$out_dir" -C cross="$pbc_config"
