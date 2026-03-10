[settings]
arch=armv8
arch.microarch=cortex-a53

[conf]
tools.build:cxxflags+=["/arch:armv8.0"]
tools.build:cflags+=["/arch:armv8.0"]

[options]
openblas/*:target=CORTEXA53
blasfeo/*:target=ARMV8A_ARM_CORTEX_A53
