cmake_minimum_required(VERSION 3.21)
project(alpaqa-examples C CXX)

if (PROJECT_IS_TOP_LEVEL)
    find_package(alpaqa REQUIRED CONFIG OPTIONAL_COMPONENTS CasADi Dl Extra)
endif()

add_subdirectory(C++)
add_subdirectory(problems)