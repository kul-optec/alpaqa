# Look for a C and Fortran compiler (optional)
include(CheckLanguage)
set(ALPAQA_HAVE_FORTRAN Off)
check_language(Fortran)
if (CMAKE_Fortran_COMPILER)
    set(ALPAQA_HAVE_FORTRAN On)
    enable_language(Fortran)
endif()
set(ALPAQA_HAVE_C Off)
check_language(C)
if (CMAKE_C_COMPILER)
    set(ALPAQA_HAVE_C On)
    enable_language(C)
endif()
# Determine compiler's C++23 support
set(ALPAQA_WITH_CXX_23_DEFAULT Off)
if ("cxx_std_23" IN_LIST CMAKE_CXX_COMPILE_FEATURES)
    set(ALPAQA_WITH_CXX_23_DEFAULT On)
endif()
# Check if the compiler is new enough for the CasADi OCP interface
set(ALPAQA_WITH_CASADI_OCP_SUPPORTED On)
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND
    CMAKE_CXX_COMPILER_VERSION VERSION_LESS 16.0.0)
    set(ALPAQA_WITH_CASADI_OCP_SUPPORTED Off)
endif()