[settings]
arch=armv8
arch.microarch=cortex-a76

[conf]
tools.build:cflags+=["-mcpu=cortex-a76"]
tools.build:cxxflags+=["-mcpu=cortex-a76"]

[options]
openblas/*:target=CORTEXA76
blasfeo/*:target=ARMV8A_ARM_CORTEX_A76
