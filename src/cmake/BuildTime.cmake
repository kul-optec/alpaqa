string(TIMESTAMP ALPAQA_BUILD_TIME UTC)
configure_file(${CMAKE_CURRENT_LIST_DIR}/alpaqa-build-time.cpp.in
    alpaqa-build-time.cpp @ONLY)
