option(USE_GLOBAL_PYBIND11 "Don't query Python to find pybind11" Off)
mark_as_advanced(USE_GLOBAL_PYBIND11)

# First tries to find Python 3, then tries to import the pybind11 module to
# query the CMake config location, and finally imports pybind11 using
# find_package(pybind11 REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH).
function(find_pybind11_python_first)

    # Find Python
    if (CMAKE_CROSSCOMPILING AND FALSE)
        find_package(Python3 REQUIRED COMPONENTS Development.Module)
    else()
        find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    endif()

    # Tweak extension suffix and debug ABI when cross-compiling
    if (CMAKE_CROSSCOMPILING)
        if (NOT PY_BUILD_EXT_SUFFIX AND DEFINED TOOLCHAIN_Python3_EXT_SUFFIX)
            set(PY_BUILD_EXT_SUFFIX ${TOOLCHAIN_Python3_EXT_SUFFIX})
        endif()
        # SETUPTOOLS_EXT_SUFFIX environment variable
        if (NOT PY_BUILD_EXT_SUFFIX AND DEFINED ENV{SETUPTOOLS_EXT_SUFFIX})
            message(STATUS "Setting PY_BUILD_EXT_SUFFIX to "
                "ENV{SETUPTOOLS_EXT_SUFFIX}: $ENV{SETUPTOOLS_EXT_SUFFIX}")
            set(PY_BUILD_EXT_SUFFIX $ENV{SETUPTOOLS_EXT_SUFFIX})
        endif()
        # If that still didn't work, use the Python3_SOABI variable:
        if (NOT PY_BUILD_EXT_SUFFIX AND Python3_SOABI)
            message(STATUS "Determining Python extension suffix based on "
                    "Python3_SOABI.")
            if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
                set(PY_BUILD_EXTENSION ".pyd")
            else()
                set(PY_BUILD_EXTENSION "${CMAKE_SHARED_MODULE_SUFFIX}")
            endif()
            set(PY_BUILD_EXT_SUFFIX ".${Python3_SOABI}${PY_BUILD_EXTENSION}")
        endif()
        # Sanity checks:
        if (NOT PY_BUILD_EXT_SUFFIX)
            message(FATAL_ERROR "Unable to determine extension suffix.\
                Try manually setting PY_BUILD_EXT_SUFFIX.")
        endif()
        if (Python3_SOABI AND
                NOT PY_BUILD_EXT_SUFFIX MATCHES "\\.${Python3_SOABI}\\.")
            message(WARNING "PY_BUILD_EXT_SUFFIX (${PY_BUILD_EXT_SUFFIX}) "
                "does not match Python3_SOABI (${Python3_SOABI})")
        endif()
        # Check the debug ABI:
        if (NOT PY_BUILD_DEBUG_ABI AND DEFINED TOOLCHAIN_Python3_DEBUG_ABI)
            set(PY_BUILD_DEBUG_ABI ${TOOLCHAIN_Python3_DEBUG_ABI})
        endif()
        # Otherwise, try to deduce it from the SOABI:
        if (NOT DEFINED PY_BUILD_DEBUG_ABI)
            if (PY_BUILD_EXT_SUFFIX MATCHES "[0-9]+d-")
                set(PY_BUILD_DEBUG_ABI true)
            else()
                set(PY_BUILD_DEBUG_ABI false)
            endif()
        endif()
        # Cache the result:
        set(PY_BUILD_EXT_SUFFIX ${PY_BUILD_EXT_SUFFIX} CACHE STRING
            "The extension for Python extension modules")
        set(PY_BUILD_DEBUG_ABI ${PY_BUILD_DEBUG_ABI} CACHE BOOL
            "Whether to compile for a debug version of Python")
        # Override pybind11NewTools.cmake's PYTHON_MODULE_EXTENSION variable:
        message(STATUS "Python extension suffix: ${PY_BUILD_EXT_SUFFIX}")
        message(STATUS "Python debug ABI: ${PY_BUILD_DEBUG_ABI}")
        set(PYTHON_MODULE_EXTENSION ${PY_BUILD_EXT_SUFFIX}
            CACHE INTERNAL "" FORCE)
        set(PYTHON_IS_DEBUG ${PY_BUILD_DEBUG_ABI}
            CACHE INTERNAL "" FORCE)
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

endfunction()