add_library(example-util "src/stdout.cpp")
target_include_directories(example-util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
add_library(alpaqa::example-util ALIAS example-util)
if (DEFINED PICO_SDK_VERSION_STRING)
    target_link_libraries(example-util PUBLIC pico_stdlib pico_stdio_usb)
    target_compile_definitions(example-util PRIVATE PICO)
endif()