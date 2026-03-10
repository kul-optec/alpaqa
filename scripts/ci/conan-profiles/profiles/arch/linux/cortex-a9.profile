[settings]
arch=armv7hf
arch.microarch=cortex-a9

[conf]
tools.build:cflags+=["-mcpu=cortex-a9"]
tools.build:cxxflags+=["-mcpu=cortex-a9"]

[options]
openblas/*:target=ARMV7
blasfeo/*:target=ARMV7A_ARM_CORTEX_A9
