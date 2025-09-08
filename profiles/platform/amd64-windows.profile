include({{ os.path.join(profile_dir, "_windows.profile") }})

[settings]
arch=x86_64

[conf]
tools.build:cxxflags+=["/arch:AVX2"]
tools.build:cflags+=["/arch:AVX2"]
