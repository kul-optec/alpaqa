[settings]
arch=x86_64
arch.microarch=avx-512

[conf]
tools.build:cxxflags+=["/arch:AVX512"]
tools.build:cflags+=["/arch:AVX512"]

[options]
openblas/*:target=SKYLAKEX
# blasfeo/*:target=X64_INTEL_SKYLAKE_X
blasfeo/*:target=X64_INTEL_HASWELL
# HPIPM fails when using SKYLAKE_X
