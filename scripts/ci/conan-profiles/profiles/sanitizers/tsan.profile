[settings]
compiler.tsan=True

[conf]
tools.build:cflags+=["-fsanitize=thread"]
tools.build:cxxflags+=["-fsanitize=thread"]
tools.build:sharedlinkflags+=["-fsanitize=thread"]
tools.build:exelinkflags+=["-fsanitize=thread"]
