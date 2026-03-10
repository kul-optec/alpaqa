# Links libgfortran and libquadmath statically

[settings]
compiler.static-libgfortran=True
compiler.static-libquadmath=True

[conf]
tools.build:exelinkflags+=["-static-libgfortran", "-static-libquadmath"]
tools.build:sharedlinkflags+=["-static-libgfortran", "-static-libquadmath"]
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_INIT': '${CMAKE_SHARED_LINKER_FLAGS_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT': '${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT': '${CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT': '${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT}'}

[options]
coinmumps/*:static_fortran_libs=True
