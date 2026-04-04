using BinaryBuilder
using Pkg

name = "Alpaqa"
version = v"1.1.0"

sources = [
    DirectorySource(normpath(joinpath(@__DIR__, "..", ".."))),
    GitSource("https://github.com/tttapa/guanaqo.git",
              "fe7223d92aca3e40c4add51d0a21a9ac89358595"),
]

script = raw"""
set -euxo pipefail

# Use CMake and Ninja provided by jll
export PATH="${host_prefix}/bin:${PATH}"

# First, build and install guanaqo (statically)
guanaqo_build="${WORKSPACE}/build-guanaqo"
guanaqo_prefix="${prefix}/share/alpaqa/vendored"
_prefix="${prefix}"
prefix="${guanaqo_prefix}"
cmake -S "${WORKSPACE}/srcdir/guanaqo" -B "${guanaqo_build}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=${guanaqo_prefix} \
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} \
    -DCMAKE_PREFIX_PATH=${prefix} \
    -DCMAKE_FIND_ROOT_PATH=${prefix} \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -DBUILD_SHARED_LIBS=Off \
    -DCMAKE_POSITION_INDEPENDENT_CODE=On \
    -DCMAKE_VISIBILITY_INLINES_HIDDEN=On \
    -DCMAKE_CXX_VISIBILITY_PRESET=hidden \
    -DCMAKE_C_VISIBILITY_PRESET=hidden \
    -DBUILD_TESTING=Off

cmake --build "${guanaqo_build}" --parallel ${nproc}
cmake --install "${guanaqo_build}"

# Now build alpaqa with access to guanaqo
prefix="${_prefix}"
alpaqa_build="${WORKSPACE}/build-alpaqa"
cmake -S ${WORKSPACE}/srcdir -B "${alpaqa_build}" \
    -G Ninja \
    -DCMAKE_BUILD_TYPE=Release \
    -DCMAKE_INSTALL_PREFIX=${prefix} \
    -DCMAKE_TOOLCHAIN_FILE=${CMAKE_TARGET_TOOLCHAIN} \
    -DCMAKE_PREFIX_PATH=${prefix} \
    -DCMAKE_FIND_ROOT_PATH=${prefix} \
    -DCMAKE_FIND_ROOT_PATH_MODE_PACKAGE=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_LIBRARY=ONLY \
    -DCMAKE_FIND_ROOT_PATH_MODE_INCLUDE=ONLY \
    -Dguanaqo_DIR=${guanaqo_prefix}/lib/cmake/guanaqo \
    -DBUILD_SHARED_LIBS=On \
    -DCMAKE_POSITION_INDEPENDENT_CODE=On \
    -DCMAKE_VISIBILITY_INLINES_HIDDEN=On \
    -DCMAKE_CXX_VISIBILITY_PRESET=hidden \
    -DCMAKE_C_VISIBILITY_PRESET=hidden \
    -DBUILD_TESTING=Off \
    -DALPAQA_WITH_JULIA=On \
    -DALPAQA_WITH_DRIVERS=On \
    -DALPAQA_WITH_EXAMPLES=Off \
    -DALPAQA_WITH_DL=On \
    -DALPAQA_WITH_JSON=On \
    -DALPAQA_WITH_CASADI=On \
    -DALPAQA_WITH_EXTERNAL_CASADI=Off \
    -DALPAQA_WITH_IPOPT=On \
    -DALPAQA_WITH_QPALM=Off \
    -DALPAQA_WITH_LBFGSB=On \
    -DALPAQA_WITH_CUTEST=On

cmake --build "${alpaqa_build}" --parallel ${nproc}
cmake --install "${alpaqa_build}"
"""

platforms = supported_platforms()
platforms = expand_cxxstring_abis(platforms)

products = [
    LibraryProduct("alpaqa-jl", :alpaqa_jl),
]

dependencies = [
    Dependency("Ipopt_jll"; compat="300.1400"),
    Dependency("Eigen_jll"; compat="3.4"),
    Dependency("nlohmann_json_jll"; compat="3.12"),
    Dependency("CompilerSupportLibraries_jll"; compat="1.1"),
    HostBuildDependency(PackageSpec(; name="CMake_jll", version=v"3.31.9+0")),
    HostBuildDependency(PackageSpec(; name="Ninja_jll", version=v"1.13.1+0")),
]

build_tarballs(ARGS, name, version, sources, script, platforms, products, dependencies;
               julia_compat="1.6",
               preferred_gcc_version=v"12")
