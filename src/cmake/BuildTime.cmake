string(TIMESTAMP ALPAQA_BUILD_TIME UTC)
configure_file(${CMAKE_CURRENT_LIST_DIR}/alpaqa-build-time.c.in
    alpaqa-build-time.c @ONLY)
