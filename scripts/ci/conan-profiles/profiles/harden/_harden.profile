[settings]
compiler.harden=true

[conf]
tools.build:cflags+=["-fstack-protector-all", "-mshstk", "-fcf-protection=full"]
tools.build:cxxflags+=["-fstack-protector-all", "-mshstk", "-fcf-protection=full"]
