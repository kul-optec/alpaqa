[settings]
compiler.asan=True

[conf]
tools.build:cflags+=["-fsanitize=address"]
tools.build:cxxflags+=["-fsanitize=address"]
tools.build:sharedlinkflags+=["-fsanitize=address"]
tools.build:exelinkflags+=["-fsanitize=address"]
