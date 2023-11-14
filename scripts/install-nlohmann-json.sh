#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/..

prefix="$1" # Installation directory prefix
build_type="${2:-RelWithDebInfo}"
version="3.11.2"

if [ -z "$prefix" ]; then
    if [ -z "$VIRTUAL_ENV" ]; then
        echo "No active virtual environment, refusing to install."
        exit 1
    else
        prefix="$VIRTUAL_ENV"
    fi
fi

set -ex
export CMAKE_PREFIX_PATH="$prefix:$CMAKE_PREFIX_PATH"
export PKG_CONFIG_PATH="$prefix/lib/pkgconfig:$PKG_CONFIG_PATH"

mkdir -p "$prefix/src"
pushd "$prefix/src"

# CasADi
[ -d nlohmann-json ] \
 || git clone --single-branch --depth=1 --branch "v$version" \
    https://github.com/nlohmann/json nlohmann-json
pushd nlohmann-json
cmake -S . -B build \
    -G "Ninja Multi-Config" \
    -D CMAKE_INSTALL_PREFIX="$prefix" \
    -D CMAKE_POSITION_INDEPENDENT_CODE=On \
    -D JSON_MultipleHeaders=On \
    -D JSON_BuildTests=Off \
    -D JSON_Install=On \
    -D CMAKE_OSX_ARCHITECTURES="arm64;x86_64"
cmake --build build -j --config $build_type
cmake --install build --config $build_type
popd

popd
