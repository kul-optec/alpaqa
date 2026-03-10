[settings]
arch=armv8
arch.microarch=cortex-a72

[conf]
tools.build:cflags+=["-mcpu=cortex-a72"]
tools.build:cxxflags+=["-mcpu=cortex-a72"]

[options]
openblas/*:target=CORTEXA72
blasfeo/*:target=ARMV8A_ARM_CORTEX_A57
# No specific target for the Cortex-A72 in BLASFEO
