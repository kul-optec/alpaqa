[settings]
# armv6hf doesn't exist
arch=armv6
os=Linux

[conf]
tools.build:cflags+=["-mfpu=vfp", "-mfloat-abi=hard"]
tools.build:cxxflags+=["-mfpu=vfp", "-mfloat-abi=hard"]

[options]
openblas/*:target=ARMV6
