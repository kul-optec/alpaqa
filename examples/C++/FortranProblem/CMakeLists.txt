add_library(problem-fortran problem.f90)
add_executable(fortran-problem-example main.cpp)
target_link_libraries(fortran-problem-example
    PRIVATE alpaqa::alpaqa alpaqa::warnings problem-fortran)
set_target_properties(fortran-problem-example PROPERTIES
    LINKER_LANGUAGE Fortran)
