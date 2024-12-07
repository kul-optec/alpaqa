include({{ os.path.join(profile_dir, "cross-linux.profile") }})

[settings]
# armv6hf doesn't exist
arch=armv6
os=Linux

[conf]
tools.build:cflags+=["-mfpu=vfp", "-mfloat-abi=hard"]
tools.build:cxxflags+=["-mfpu=vfp", "-mfloat-abi=hard"]
tools.build:exelinkflags+=["-latomic"]
tools.build:sharedlinkflags+=["-latomic"]

[options]
openblas/*:target=ARMV6
