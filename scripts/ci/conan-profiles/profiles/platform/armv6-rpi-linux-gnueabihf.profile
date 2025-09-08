include({{ os.path.join(profile_dir, "_cross-linux.profile") }})

[settings]
# armv6hf doesn't exist
arch=armv6
os=Linux
os.toolchain-vendor=rpi

[conf]
tools.build:cflags+=["-mfpu=vfp", "-mfloat-abi=hard"]
tools.build:cxxflags+=["-mfpu=vfp", "-mfloat-abi=hard"]
tools.cmake.cmaketoolchain:extra_variables*={"CPACK_DEBIAN_PACKAGE_ARCHITECTURE": "armhf"}
tools.build:exelinkflags+=["-latomic"]
tools.build:sharedlinkflags+=["-latomic"]

[options]
openblas/*:target=ARMV6
