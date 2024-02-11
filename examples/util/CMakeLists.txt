add_library(example-util "src/stdout.cpp")
target_include_directories(example-util PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)
add_library(alpaqa::example-util ALIAS example-util)