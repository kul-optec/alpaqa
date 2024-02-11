cmake_minimum_required(VERSION 3.21)
project(alpaqa-examples C CXX)

if (PROJECT_IS_TOP_LEVEL)
    find_package(alpaqa REQUIRED CONFIG OPTIONAL_COMPONENTS CasADi Dl Extra)
endif()

add_subdirectory(util)

add_custom_target(examples ALL)
function(alpaqa_register_example TGT)
    target_link_libraries(${TGT} PRIVATE alpaqa::example-util)
    add_dependencies(examples ${TGT})
    if (DEFINED PICO_SDK_VERSION_STRING)
        pico_enable_stdio_usb(${TGT} 1)
        pico_enable_stdio_uart(${TGT} 0)
        pico_add_extra_outputs(${TGT})
    endif()
endfunction()

add_subdirectory(C++)
add_subdirectory(problems)