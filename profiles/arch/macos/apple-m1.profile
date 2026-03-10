# Note: please do not use this profile directly, as it does not enable target-specific tuning.
[settings]
arch=armv8
arch.microarch=apple-m1

[conf]
tools.build:cxxflags+=["-mcpu=apple-m1"]
tools.build:cflags+=["-mcpu=apple-m1"]

[options]
openblas/*:target=VORTEX
blasfeo/*:target=ARMV8A_APPLE_M1
