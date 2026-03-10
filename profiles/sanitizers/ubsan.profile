[settings]
compiler.ubsan=True

[conf]
tools.build:cflags+=["-fsanitize=undefined"]
tools.build:cxxflags+=["-fsanitize=undefined"]
tools.build:sharedlinkflags+=["-fsanitize=undefined"]
tools.build:exelinkflags+=["-fsanitize=undefined"]
