[settings]
arch=armv8
arch.microarch=cortex-a53

[conf]
tools.build:cflags+=["-mcpu=cortex-a53"]
tools.build:cxxflags+=["-mcpu=cortex-a53"]

[options]
openblas/*:target=CORTEXA53
blasfeo/*:target=ARMV8A_ARM_CORTEX_A53
