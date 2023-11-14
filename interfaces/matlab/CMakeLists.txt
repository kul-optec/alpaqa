# Find the Matlab MEX compiler:
find_package(Matlab REQUIRED COMPONENTS MEX_COMPILER)

# Matlab uses UTF-16 for all its strings, so we need to convert back and forth
find_package(utf8cpp REQUIRED)

# Matlab interface requires CasADi and JSON
if (NOT TARGET alpaqa::casadi-loader)
    message(FATAL_ERROR "MATLAB MEX interface requires CasADi")
endif()
if (NOT ALPAQA_WITH_JSON)
    message(FATAL_ERROR "MATLAB MEX interface requires JSON")
endif()

# Create a target for compiling a MEX function:
matlab_add_mex(NAME alpaqa_mex
    SRC
        "src/alpaqa_mex.cpp" "src/solve.cpp" "src/unicode.cpp"
    LINK_TO
        alpaqa::alpaqa alpaqa::casadi-loader utf8cpp::utf8cpp
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
