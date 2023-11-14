# Find the Matlab MEX compiler:
find_package(Matlab REQUIRED COMPONENTS MEX_COMPILER)

# Create a target for compiling a MEX function:
matlab_add_mex(
    NAME alpaqa_mex                                 # Name for use in Matlab
    SRC "src/alpaqa_mex.cpp" "src/solve.cpp"        # Source files to compile
    LINK_TO alpaqa::casadi-loader                   # Dependencies
)
target_include_directories(alpaqa_mex PRIVATE include)
if (NOT WIN32 AND NOT APPLE)
    # Matlab's C++ standard library is too old, so link it statically:
    target_link_options(alpaqa_mex PUBLIC -static-libstdc++ -static-libgcc)
endif()

# Install the final MEX file
install(FILES $<TARGET_FILE:alpaqa_mex>
        "minimize.m" "Problem.m"
        EXCLUDE_FROM_ALL
        COMPONENT mex_interface
        DESTINATION "+alpaqa")
