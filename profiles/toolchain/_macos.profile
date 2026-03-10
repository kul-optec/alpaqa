include(default)

[settings]
os=Macos
build_type=Release
compiler.cppstd=23
os.version=11.0

[conf]
tools.build:compiler_executables*={"fortran": "FC-NOTFOUND" }

[buildenv]
MACOSX_DEPLOYMENT_TARGET=11.0
