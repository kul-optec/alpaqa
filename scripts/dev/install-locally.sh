#!/usr/bin/env bash
cd "$( dirname "${BASH_SOURCE[0]}" )"/../..

set -ex

# Select architecture
triple="x86_64-centos7-linux-gnu"

# Download compiler
download_url="https://github.com/tttapa/cross-python/releases/download/1.0.0"
tools_dir="$PWD/toolchains"
pfx="$tools_dir/$triple"
mkdir -p "$tools_dir"
if [ ! -d "$pfx" ]; then
    wget "$download_url/full-$triple.tar.xz" -O- | \
        tar xJ -C "$tools_dir"
fi

# Install QPALM
if [ ! -d "$pfx/qpalm" ]; then
    qpalm_download="https://github.com/kul-optec/QPALM/releases/download/1.2.5"
    wget "$qpalm_download/QPALM-1.2.5-Linux.tar.gz" -O- | \
        tar xz -C "$pfx"
    mv "$pfx/QPALM-1.2.5-Linux" "$pfx/qpalm"
fi

# Install guanaqo
if [ ! -d "$pfx/guanaqo" ]; then
    mkdir -p "$pfx/download"
    git clone git@github.com:tttapa/guanaqo.git "$pfx/download/guanaqo"
    cmake -B "$pfx/download/guanaqo/build" -S "$pfx/download/guanaqo" -G "Ninja Multi-Config" \
        -D BUILD_TESTING=Off -D CMAKE_STAGING_PREFIX="$pfx/guanaqo/usr/local" \
        --toolchain "$pfx/$triple.toolchain.cmake"
    for cfg in Debug RelWithDebInfo; do cmake --build "$pfx/download/guanaqo/build" -j --config $cfg; done
    for cfg in Debug RelWithDebInfo; do cmake --install "$pfx/download/guanaqo/build" --config $cfg; done
    for cfg in Debug RelWithDebInfo; do cmake --install "$pfx/download/guanaqo/build" --config $cfg --component debug; done
fi

# Configure
cmake --preset dev-linux-cross-native
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
config="$triple.py-build-cmake.config.toml"
cat <<- EOF > "$config"
[cmake]
config = ["Debug", "Release"]
generator = "Ninja Multi-Config"
preset = "dev-linux-cross-native-python"
EOF
. ./.venv/bin/activate
pip install -U pip build
develop=false
if $develop; then
    pip install -e ".[test]" -v \
        --config-settings=--cross="$pfx/$triple.py-build-cmake.cross.toml" \
        --config-settings=--local="$PWD/$config"
else
    python -m build -w "." -o staging \
        -C--cross="$pfx/$triple.py-build-cmake.cross.toml" \
        -C--local="$PWD/$config"
    python -m build -w "python/alpaqa-debug" -o staging \
        -C--cross="$pfx/$triple.py-build-cmake.cross.toml" \
        -C--local="$PWD/$config"
    pip install -f staging --force-reinstall --no-deps \
        "alpaqa==1.0.0a20" "alpaqa-debug==1.0.0a20"
    pip install -f staging \
        "alpaqa[test]==1.0.0a20" "alpaqa-debug==1.0.0a20"
fi
pytest
