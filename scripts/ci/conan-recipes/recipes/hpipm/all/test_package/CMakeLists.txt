cmake_minimum_required(VERSION 3.15)
project(PackageTest C)

find_package(hpipm CONFIG REQUIRED)
find_library(MATH_LIBRARY m)

add_executable(example
    src/example_d_dense_qp.c
    src/data/getting_started_dense_qp_data.c)
target_link_libraries(example PRIVATE hpipm)
if(MATH_LIBRARY)
    target_link_libraries(example PRIVATE ${MATH_LIBRARY})
endif()
if (HPIPM_VERSION STREQUAL "tttapa.20251105")
    target_compile_definitions(example PRIVATE HPIPM_NO_NSB)
endif()
