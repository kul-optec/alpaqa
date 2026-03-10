#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..

set -ex

# Matlab folder
matlab_dir="${1:-/usr/local/MATLAB}"

# Select architecture
triple="${2:-x86_64-bionic-linux-gnu}"
case $triple in
    x86_64-centos7-*) arch=linux/x86-64-v3 ;;
    x86_64-bionic-*) arch=linux/x86-64-v3 ;;
    arm64-macos) arch=macos/apple-m1 ;;
    x86_64-macos) arch=macos/x86-64-v3 ;;
    *) echo "Unknown platform ${triple}"; exit 1 ;;
esac

# Package and output directories
pkg_dir="${3:-.}"
out_dir="${4:-staging/matlab}"
shared="${5:-False}"

# Create Conan profiles
matlab_profile="$PWD/profile-matlab.local.conan"
profiles="$PWD/scripts/ci/conan-profiles/profiles"
cat <<- EOF > "$matlab_profile"
include($profiles/toolchain/$triple.profile)
include($profiles/arch/$arch.profile)
include($profiles/test/none.profile)
include($PWD/scripts/ci/options/alpaqa-matlab.profile)
EOF
if [[ "$OSTYPE" == "linux-gnu"* ]]; then
    cat <<- EOF >> "$matlab_profile"
	include($profiles/gcc-static.profile)
	EOF
else
    cat <<- EOF >> "$matlab_profile"
	include($profiles/sccache/only-self.profile)
	EOF
fi

# Install dependencies
rm -rf "$pkg_dir"/build/matlab-{debug,release}/{generators,CMakeCache.txt}
for cfg in Release; do
    conan install "$pkg_dir" --build=missing \
        -pr:h "$matlab_profile" \
        -s build_type=$cfg \
        -o alpaqa/\*:shared=$shared \
        -c \&:tools.cmake.cmaketoolchain:generator=Ninja
done

# Build MATLAB bindings
pushd "$pkg_dir"
cmake --preset conan-matlab-release \
    -D CMAKE_FIND_ROOT_PATH="$matlab_dir" \
    -D CMAKE_C_COMPILER_LAUNCHER=sccache \
    -D CMAKE_CXX_COMPILER_LAUNCHER=sccache \
    -D ALPAQA_INSTALL_MEXDIR="/" \
    -D ALPAQA_INSTALL_LIBDIR="/alpaqa.lib" --fresh
cmake --build --preset conan-matlab-release -v
for component in lib casadi extra matlab; do
    DESTDIR="$out_dir" \
    cmake --install build/matlab-release --component $component
done
popd
