# Adds the -fvisibility=hidden and -fvisibility-inlines-hidden flags

[settings]
compiler.hidden=true

[conf]
tools.build:cxxflags+=["-fvisibility=hidden", "-fvisibility-inlines-hidden"]
tools.build:cflags+=["-fvisibility=hidden"]
[buildenv]
FFLAGS+= -fvisibility=hidden
