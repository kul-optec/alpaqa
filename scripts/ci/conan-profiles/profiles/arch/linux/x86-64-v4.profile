# Note: please do not use this profile directly, as it does not enable target-specific tuning.
[settings]
arch=x86_64
arch.microarch=x86-64-v4

[conf]
tools.build:cflags+=["-march=x86-64-v4"]
tools.build:cxxflags+=["-march=x86-64-v4"]

[options]
openblas/*:target=SKYLAKEX
# blasfeo/*:target=X64_INTEL_SKYLAKE_X
blasfeo/*:target=X64_INTEL_HASWELL
# HPIPM fails when using SKYLAKE_X
