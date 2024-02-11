add_executable(lasso-fbs lasso-fbs.cpp)
target_link_libraries(lasso-fbs PRIVATE alpaqa::alpaqa alpaqa::warnings)
alpaqa_register_example(lasso-fbs)