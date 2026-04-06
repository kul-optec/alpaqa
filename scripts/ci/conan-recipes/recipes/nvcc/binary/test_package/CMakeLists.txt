cmake_minimum_required(VERSION 3.18)
if (NOT DEFINED CMAKE_CUDA_ARCHITECTURES)
    set(CMAKE_CUDA_ARCHITECTURES "50")
endif()
project(PackageTest CXX CUDA)

find_package(CUDAToolkit REQUIRED)

add_executable(example "src/main.cpp" "src/cuda_add.cu")
