[settings]
arch=x86_64
arch.microarch=avx2

[conf]
tools.build:cxxflags+=["/arch:AVX2"]
tools.build:cflags+=["/arch:AVX2"]

[options]
openblas/*:target=HASWELL
blasfeo/*:target=X64_INTEL_HASWELL
