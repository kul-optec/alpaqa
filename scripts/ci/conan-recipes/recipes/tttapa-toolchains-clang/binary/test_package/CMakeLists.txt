cmake_minimum_required(VERSION 3.15)
project(PackageTest C CXX)

find_package(OpenMP REQUIRED COMPONENTS C CXX)

add_executable(example src/example.cpp)
target_link_libraries(example OpenMP::OpenMP_CXX)

add_executable(example_c src/example.c)
target_link_libraries(example_c OpenMP::OpenMP_C)

# No Fortran support for now, since Conan does not support Flang
