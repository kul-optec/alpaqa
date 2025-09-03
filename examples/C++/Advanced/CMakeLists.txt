add_executable(lasso-fbs lasso-fbs.cpp)
target_link_libraries(lasso-fbs PRIVATE alpaqa::alpaqa alpaqa::warnings)
alpaqa_register_example(lasso-fbs)

add_executable(custom-vec-type custom-vec-type.cpp)
target_link_libraries(custom-vec-type PRIVATE alpaqa::alpaqa alpaqa::warnings)
alpaqa_register_example(custom-vec-type)
