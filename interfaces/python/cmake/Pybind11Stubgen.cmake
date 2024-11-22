function(pybind11_stubgen target)

    cmake_parse_arguments(STUBGEN "" "PACKAGE;DESTINATION;COMPONENT" "" ${ARGN})
    set(STUBGEN_COMPONENT "python_stubs")

    if (NOT DEFINED Python3_EXECUTABLE)
        find_package(Python3 REQUIRED COMPONENTS Interpreter)
    endif()

    set(STUBGEN_MODULE $<TARGET_FILE_BASE_NAME:${target}>)
    set(STUBGEN_CMD "\"${Python3_EXECUTABLE}\" -m pybind11_stubgen -o \"${ALPAQA_INSTALL_PYSTUBSDIR}\" 
        --exit-code --numpy-array-use-type-var --enum-class-locations Sign:LBFGS
        \"${STUBGEN_MODULE}\"")
    install(CODE "
        execute_process(COMMAND ${STUBGEN_CMD}
                        WORKING_DIRECTORY \"\${CMAKE_INSTALL_PREFIX}/${PY_BUILD_CMAKE_MODULE_NAME}\"
                        RESULT_VARIABLE STUBGEN_RET)
        if(NOT STUBGEN_RET EQUAL 0)
            message(SEND_ERROR \"pybind11-stubgen ${STUBGEN_MODULE} failed.\")
        endif()
        " EXCLUDE_FROM_ALL COMPONENT ${STUBGEN_COMPONENT})

endfunction()
