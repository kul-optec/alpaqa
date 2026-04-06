cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

if (DEFINED ENV{MKLROOT})
    list(PREPEND CMAKE_FIND_ROOT_PATH "$ENV{MKLROOT}")
endif()

find_package(hyhound CONFIG REQUIRED OPTIONAL_COMPONENTS OCP)

add_executable(example src/example.cpp)
target_link_libraries(example PRIVATE hyhound::hyhound)
if (TARGET hyhound::ocp)
    target_link_libraries(example PRIVATE hyhound::ocp)
endif()
