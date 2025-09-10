# Links libstdc++, libgfortran, libquadmath and libgcc statically (but not libgcc_eh)

[settings]
compiler.static-libs=stdc++ gfortran quadmath gcc

[conf]
tools.build:exelinkflags+=["-static-libstdc++", "-static-libgfortran", "-static-libquadmath"]
tools.build:sharedlinkflags+=["-static-libstdc++", "-static-libgfortran", "-static-libquadmath"]
tools.cmake.cmaketoolchain:user_toolchain=+['{{ os.path.join(profile_dir, "static-libgcc.cmake") }}']
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_INIT': '${CMAKE_SHARED_LINKER_FLAGS_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT': '${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT': '${CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT': '${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT}'}

[options]
coinmumps/*:static_fortran_libs=True
