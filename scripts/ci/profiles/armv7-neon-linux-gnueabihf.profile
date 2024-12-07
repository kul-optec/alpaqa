include({{ os.path.join(profile_dir, "cross-linux.profile") }})

[settings]
arch=armv7hf
os=Linux

[conf]
tools.build:cflags+=["-mfpu=neon", "-mfloat-abi=hard"]
tools.build:cxxflags+=["-mfpu=neon", "-mfloat-abi=hard"]

[options]
openblas/*:target=ARMV7
