include(default)

[settings]
os=Windows
build_type=Release
compiler.cppstd=23

[conf]
tools.build:compiler_executables*={"fortran": "FC-NOTFOUND" }
