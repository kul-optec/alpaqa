# Links libstdc++, libgfortran, libquadmath and libgcc statically (but not libgcc_eh)

[settings]
compiler.static-libs=stdc++ gfortran quadmath gcc

[conf]
tools.build:exelinkflags+=["-static-libstdc++", "-static-libgfortran", "-static-libquadmath"]
tools.build:sharedlinkflags+=["-static-libstdc++", "-static-libgfortran", "-static-libquadmath"]
tools.cmake.cmaketoolchain:user_toolchain=+['{{ os.path.join(profile_dir, "static-libgcc.cmake") }}']

[options]
coinmumps/*:static_fortran_libs=True
