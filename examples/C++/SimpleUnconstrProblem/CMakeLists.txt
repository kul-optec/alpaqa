add_executable(simple-unconstr-cpp-problem-example main.cpp)
target_link_libraries(simple-unconstr-cpp-problem-example
    PRIVATE alpaqa::alpaqa alpaqa::warnings)
alpaqa_register_example(simple-unconstr-cpp-problem-example)