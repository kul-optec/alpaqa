include({{ os.path.join(profile_dir, "linux-conf.profile") }})

[settings]
build_type=Release
compiler=gcc
compiler.cppstd=gnu23
compiler.libcxx=libstdc++11
compiler.version=14

[tool_requires]
tttapa-toolchains/1.0.1
