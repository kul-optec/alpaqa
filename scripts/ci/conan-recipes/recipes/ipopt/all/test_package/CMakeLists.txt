cmake_minimum_required(VERSION 3.15)
project(PackageTest CXX)

find_package(Ipopt CONFIG REQUIRED)

add_executable(example src/hs071_nlp.hpp src/hs071_nlp.cpp src/hs071_main.cpp)
target_link_libraries(example Ipopt::Ipopt)
