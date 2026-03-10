[settings]
compiler.sanitizers+=["leak"]

[conf]
tools.build:exelinkflags+=["-fsanitize=leak"]
