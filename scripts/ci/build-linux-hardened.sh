#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..
set -ex

# Package and output directories
pkg_dir="${1:-.}"
out_dir="${2:-dist}"
build_type="${3:-debug}"  # debug, release or relwithdebinfo

# Create Conan profiles
hardened_profile="$PWD/profile-hardened.local.conan"
profiles="$PWD/scripts/ci/conan-profiles/profiles"
cat <<- EOF > "$hardened_profile"
include(default)
include($profiles/color/gcc.profile)
include($profiles/link/lto-auto.profile)
include($profiles/visibility/hidden.profile)
include($profiles/sccache/only-self.profile)
include($profiles/test/only-self.profile)
include($profiles/harden/$build_type.profile)
include($profiles/tools/ninja.profile)
include($PWD/scripts/ci/options/alpaqa-python-linux.profile)
[conf]
tools.cmake.cmaketoolchain:generator=Ninja Multi-Config
EOF
if [ "$build_type" = "debug" ]; then
    export ALPAQA_PYTHON_DEBUG=1
fi

# Create a py-build-cmake config file
pbc_config="$PWD/native-py-build-cmake.local.pbc"
cat << EOF > "$pbc_config"
conan.profile_host=["$hardened_profile"]
conan.cmake.args+=["--fresh"]
conan.cmake.build_args+=["--verbose"]
conan.cmake.build_type=!
EOF

# Build the Python package
python3 -m pip install -U build
python3 -m build -w "$pkg_dir" -o "$out_dir" -C local="$pbc_config"
