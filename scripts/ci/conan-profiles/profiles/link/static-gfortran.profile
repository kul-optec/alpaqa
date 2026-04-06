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

[buildenv]
FFLAGS+= -static-libgfortran -static-libquadmath
# The exelinkflags and sharedlinkflags above do not get propagated to the point
# where CMake determines the CMAKE_Fortran_IMPLICIT_LINK_LIBRARIES, so CMake
# tries to link against the shared libgfortran and libquadmath, which is not
# what we want. For some reason, the CMAKE_Fortran_STANDARD_LIBRARIES_INIT gets
# ignored, while the CMAKE_{C,CXX}_STANDARD_LIBRARIES_INIT do get used.
# That's why we set FFLAGS here as well, so CMake at least picks the static
# libraries by default. Note that this only affects libraries that are linked
# implicitly by GFortran: if any dependency explicitly links against libgfortran
# or libquadmath, the shared version might still end up in the final link line.
# TODO: make sure that linking to libgcc is consistent.

[options]
coinmumps/*:static_fortran_libs=True
openblas/*:shared_libgfortran=False
