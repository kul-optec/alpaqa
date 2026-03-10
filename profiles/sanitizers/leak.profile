[settings]
compiler.lsan=True

[conf]
tools.build:exelinkflags+=["-fsanitize=leak"]
