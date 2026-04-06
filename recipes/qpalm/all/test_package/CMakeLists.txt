cmake_minimum_required(VERSION 3.15)
project(qpalm_test_package C)

find_package(QPALM REQUIRED)
add_executable(example src/example.c)
target_link_libraries(example PRIVATE QPALM::qpalm)
install(TARGETS example)
