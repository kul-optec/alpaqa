# Links libstdc++ and libgcc statically (but not libgcc_eh)

[settings]
compiler.static-libstdc++=True

[conf]
tools.build:exelinkflags+=["-static-libstdc++"]
tools.build:sharedlinkflags+=["-static-libstdc++"]
tools.cmake.cmaketoolchain:user_toolchain=+['{{ os.path.join(profile_dir, "static-libgcc.cmake") }}']
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_INIT': '${CMAKE_SHARED_LINKER_FLAGS_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT': '${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT': '${CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT}'}
tools.cmake.cmaketoolchain:extra_variables*={'CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT': '${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT}'}

[buildenv]
FFLAGS+= -static-libgcc
