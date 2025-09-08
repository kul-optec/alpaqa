# Compiles with individual function and data sections, and removes unused sections

[conf]
tools.build:cxxflags+=["-ffunction-sections", "-fdata-sections"]
tools.build:cflags+=["-ffunction-sections", "-fdata-sections"]
tools.build:exelinkflags+=["-Wl,--gc-sections"]
tools.build:sharedlinkflags+=["-Wl,--gc-sections"]
