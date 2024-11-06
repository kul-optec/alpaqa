option(USE_GLOBAL_PYBIND11 "Don't query Python to find pybind11" Off)
mark_as_advanced(USE_GLOBAL_PYBIND11)

# First tries to find Python 3, then tries to import the pybind11 module to
# query the CMake config location, and finally imports pybind11 using
# find_package(pybind11 ${ARGN} REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH),
# where ${ARGN} are the arguments passed to this macro.
macro(find_pybind11_python_first)

    set(PYBIND11_USE_CROSSCOMPILING On)

    # Find Python
    if (CMAKE_CROSSCOMPILING AND NOT (APPLE AND "$ENV{CIBUILDWHEEL}" STREQUAL "1"))
        find_package(Python3 REQUIRED COMPONENTS Development.Module)
    else()
        find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    endif()

    # Query Python to see if it knows where the pybind11 root is
    if (NOT USE_GLOBAL_PYBIND11 AND Python3_EXECUTABLE)
        if (NOT pybind11_ROOT OR NOT EXISTS ${pybind11_ROOT})
            message(STATUS "Detecting pybind11 CMake location")
            execute_process(COMMAND ${Python3_EXECUTABLE}
                    -m pybind11 --cmakedir
                OUTPUT_VARIABLE PY_BUILD_PYBIND11_ROOT
                OUTPUT_STRIP_TRAILING_WHITESPACE
                RESULT_VARIABLE PY_BUILD_CMAKE_PYBIND11_RESULT)
            # If it was successful
            if (PY_BUILD_CMAKE_PYBIND11_RESULT EQUAL 0)
                message(STATUS "pybind11 CMake location: ${PY_BUILD_PYBIND11_ROOT}")
                set(pybind11_ROOT ${PY_BUILD_PYBIND11_ROOT}
                    CACHE PATH "Path to the pybind11 CMake configuration." FORCE)
            else()
                unset(pybind11_ROOT CACHE)
            endif()
        endif()
    endif()

    # pybind11 is header-only, so finding a native version is fine
    find_package(pybind11 ${ARGN} REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH)

endmacro()
