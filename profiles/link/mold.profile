# https://github.com/conan-io/conan/issues/17333
[tool_requires]
!zlib/*:mold/2.40.1

[conf]
!zlib/*:tools.build:exelinkflags+=["-fuse-ld=mold"]
!zlib/*:tools.build:sharedlinkflags+=["-fuse-ld=mold"]
