cmake_minimum_required(VERSION 3.15)
project(ladel_test_package C)

find_package(LADEL REQUIRED)
add_executable(example src/example.c)
target_link_libraries(example PRIVATE LADEL::ladel)
install(TARGETS example)
