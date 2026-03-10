# Note: please do not use this profile directly, as it does not enable target-specific tuning.
[settings]
arch=x86_64
arch.microarch=x86-64-v3

[conf]
tools.build:cflags+=["-march=x86-64-v3"]
tools.build:cxxflags+=["-march=x86-64-v3"]

[options]
openblas/*:target=HASWELL
blasfeo/*:target=X64_INTEL_HASWELL
