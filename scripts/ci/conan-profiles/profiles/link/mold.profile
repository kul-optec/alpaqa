[tool_requires]
!xxhash/*:mold/2.40.1
!zlib/*:mold/2.40.1

[conf]
!xxhash/*:tools.build:exelinkflags+=["-fuse-ld=mold"]
!xxhash/*:tools.build:sharedlinkflags+=["-fuse-ld=mold"]
!zlib/*:tools.build:exelinkflags+=["-fuse-ld=mold"]
!zlib/*:tools.build:sharedlinkflags+=["-fuse-ld=mold"]
