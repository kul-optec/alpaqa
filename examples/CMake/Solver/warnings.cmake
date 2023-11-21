function(add_warnings_target tgt_name)

    # Add target that defines compiler warning options in its interface.
    add_library(${tgt_name} INTERFACE)
    
    # The warning flags we use depend on the language.

    # Note: these are the bare minimum warnings, it is highly recommended to
    #       add additional warning flags. See alpaqa/cmake/Warnings.cmake for
    #       an example.
    foreach (LANG C CXX Fortran)
        if (CMAKE_${LANG}_COMPILER_ID MATCHES "^(GNU)|(.*Clang)|(Flang)|(Intel)$")
            target_compile_options(${tgt_name} INTERFACE
                $<$<COMPILE_LANGUAGE:${LANG}>:$<BUILD_INTERFACE:-Wall -Wextra -pedantic>>)
        elseif (CMAKE_${LANG}_COMPILER_ID MATCHES "MSVC")
            target_compile_options(${tgt_name} INTERFACE
                $<$<COMPILE_LANGUAGE:${LANG}>:$<BUILD_INTERFACE:/W3>>)
        elseif (DEFINED CMAKE_${LANG}_COMPILER_ID)
            message(WARNING "No known warnings for this ${LANG} compiler (${CMAKE_${LANG}_COMPILER_ID})")
        endif()
    endforeach()

endfunction()
