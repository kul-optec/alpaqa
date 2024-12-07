#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..

set -ex

# Package and output directories
pkg_dir="${1:-.}"
out_dir="${2:-dist}"
install_stubs_dir="$3"

# Create Conan profiles
python_profile="$PWD/profile-python.local.conan"
cat <<- EOF > "$python_profile"
include(default)
include($PWD/scripts/ci/profiles/linux-conf.profile)
include($PWD/scripts/ci/profiles/alpaqa-python-linux.profile)
[conf]
tools.build:exelinkflags+=["-static-libgcc"]
tools.build:sharedlinkflags+=["-static-libgcc"]
EOF

pbc_config="$PWD/py-build-cmake.config.pbc"
cat <<- EOF > "$pbc_config"
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
    -C local="$pbc_config"
python -m build -w "$pkg_dir/python/alpaqa-debug" -o "$out_dir" \
    -C local="$PWD/scripts/ci/py-build-cmake.toml" \
    -C component="$PWD/scripts/ci/py-build-cmake.component.toml" \
    -C local="$pbc_config"

# Install the Python stubs
if [ -n "$install_stubs_dir" ]; then
    proj_dir="$PWD"
    cd "$pkg_dir"
    # We install the Python modules and stubs in the source directory
    for i in 10 20; do
        py-build-cmake \
            --local="$proj_dir/scripts/ci/py-build-cmake.toml" \
            --local="$pbc_config" \
            configure --index $i
        py-build-cmake \
            --local="$proj_dir/scripts/ci/py-build-cmake.toml" \
            --local="$pbc_config" \
            install --index $i --component python_modules \
            -- --prefix "$install_stubs_dir"
        py-build-cmake \
            --local="$proj_dir/scripts/ci/py-build-cmake.toml" \
            --local="$pbc_config" \
            install --index $i --component python_stubs \
            -- --prefix "$install_stubs_dir"
    done
    # Then we remove the binary Python modules (sdist is source only)
    while IFS= read -r f || [ -n "$f" ]; do rm -f "$f"
    done < build/python-debug/install_manifest_python_modules.txt
    while IFS= read -r f || [ -n "$f" ]; do rm -f "$f"
    done < build/python-release/install_manifest_python_modules.txt
    cd "$proj_dir"
fi
