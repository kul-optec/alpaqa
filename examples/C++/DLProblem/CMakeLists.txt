# Build the loadable problem
alpaqa_add_dl_problem_module(problem-c FILES problem.c)
target_compile_features(problem-c PRIVATE c_std_11)
target_link_libraries(problem-c PRIVATE alpaqa::warnings)

# Build the driver program that loads the problem
add_executable(dl-problem-example main.cpp)
target_link_libraries(dl-problem-example
    PRIVATE alpaqa::alpaqa alpaqa::warnings alpaqa::dl-loader)
target_compile_definitions(dl-problem-example
    PRIVATE DLPROBLEM_DLL=\"$<TARGET_FILE_NAME:problem-c>\")
add_dependencies(dl-problem-example problem-c)
alpaqa_register_example(dl-problem-example)

# Stand-alone executable to test the matrix functions used by problem-c
add_executable(test-matmul "test-matmul.c")
target_link_libraries(test-matmul PRIVATE alpaqa::dl-api alpaqa::warnings)
