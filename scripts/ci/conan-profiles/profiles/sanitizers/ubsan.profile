[settings]
compiler.ubsan=True

[conf]
tools.build:cflags+=["-fsanitize=undefined", "-mcmodel=large"]
tools.build:cxxflags+=["-fsanitize=undefined", "-mcmodel=large"]
tools.build:sharedlinkflags+=["-fsanitize=undefined", "-mcmodel=large"]
tools.build:exelinkflags+=["-fsanitize=undefined", "-mcmodel=large"]
