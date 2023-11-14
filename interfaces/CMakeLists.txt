# Python bindings
if(ALPAQA_WITH_PYTHON)
    add_subdirectory(python)
endif()
# MATLAB MEX interface
if(ALPAQA_WITH_MATLAB)
    add_subdirectory(matlab)
endif()