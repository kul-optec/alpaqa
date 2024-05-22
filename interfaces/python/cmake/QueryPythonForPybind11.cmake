option(USE_GLOBAL_PYBIND11 "Don't query Python to find pybind11" Off)
mark_as_advanced(USE_GLOBAL_PYBIND11)

# First tries to find Python 3, then tries to import the pybind11 module to
# query the CMake config location, and finally imports pybind11 using
# find_package(pybind11 ${ARGN} REQUIRED CONFIG CMAKE_FIND_ROOT_PATH_BOTH),
# where ${ARGN} are the arguments passed to this function.
function(find_pybind11_python_first)

    # Find Python
    if (CMAKE_CROSSCOMPILING AND NOT (APPLE AND "$ENV{CIBUILDWHEEL}" STREQUAL "1"))
        find_package(Python3 REQUIRED COMPONENTS Development.Module)
    else()
        find_package(Python3 REQUIRED COMPONENTS Interpreter Development.Module)
    endif()

    # Tweak extension suffix and debug ABI when cross-compiling
    if (CMAKE_CROSSCOMPILING AND NOT (DEFINED PYTHON_MODULE_DEBUG_POSTFIX
                                      AND DEFINED PYTHON_MODULE_EXTENSION
                                      AND DEFINED PYTHON_IS_DEBUG))
        include(${CMAKE_CURRENT_FUNCTION_LIST_DIR}/pybind11GuessPythonExtSuffix.cmake)
        pybind11_guess_python_module_extension(Python3)
        if (DEFINED PYTHON_MODULE_DEBUG_POSTFIX
          AND DEFINED PYTHON_MODULE_EXTENSION
          AND DEFINED PYTHON_IS_DEBUG)
            set(PYTHON_MODULE_DEBUG_POSTFIX "${PYTHON_MODULE_DEBUG_POSTFIX}" PARENT_SCOPE)
            set(PYTHON_MODULE_EXTENSION "${PYTHON_MODULE_EXTENSION}" PARENT_SCOPE)
            set(PYTHON_IS_DEBUG "${PYTHON_IS_DEBUG}" PARENT_SCOPE)
        endif()
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
