[conf]
tools.build:skip_test=true
tools.build:cflags+=["-fdiagnostics-color"]
tools.build:cxxflags+=["-fdiagnostics-color"]
tools.build:exelinkflags+=["-flto=auto", "-static-libstdc++"]
tools.build:sharedlinkflags+=["-flto=auto", "-static-libstdc++"]
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_INIT": "${CMAKE_SHARED_LINKER_FLAGS_INIT}"}
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_DEBUG_INIT": "${CMAKE_SHARED_LINKER_FLAGS_DEBUG_INIT}"}
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_RELEASE_INIT": "${CMAKE_SHARED_LINKER_FLAGS_RELEASE_INIT}"}
tools.cmake.cmaketoolchain:extra_variables*={"CMAKE_MODULE_LINKER_FLAGS_RELWITHDEBINFO_INIT": "${CMAKE_SHARED_LINKER_FLAGS_RELWITHDEBINFO_INIT}"}
tools.cmake.cmaketoolchain:generator=Ninja Multi-Config
