[settings]
compiler.asan=True

[conf]
tools.build:cflags+=["-fsanitize=address", "-mcmodel=large"]
tools.build:cxxflags+=["-fsanitize=address", "-mcmodel=large"]
tools.build:sharedlinkflags+=["-fsanitize=address", "-mcmodel=large"]
tools.build:exelinkflags+=["-fsanitize=address", "-mcmodel=large"]
