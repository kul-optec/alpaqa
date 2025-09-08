include(default)

[settings]
compiler.cppstd=23
os.version=11.0

[conf]
tools.cmake.cmaketoolchain:generator=Ninja Multi-Config
tools.build:compiler_executables*={"fortran": "FC-NOTFOUND" }

[buildenv]
MACOSX_DEPLOYMENT_TARGET=11.0
