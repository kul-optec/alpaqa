[settings]
compiler.tsan=True

[conf]
tools.build:cflags+=["-fsanitize=thread", "-mcmodel=large"]
tools.build:cxxflags+=["-fsanitize=thread", "-mcmodel=large"]
tools.build:sharedlinkflags+=["-fsanitize=thread", "-mcmodel=large"]
tools.build:exelinkflags+=["-fsanitize=thread", "-mcmodel=large"]
