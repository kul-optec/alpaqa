string(TIMESTAMP ALPAQA_BUILD_TIME UTC)
execute_process(
    COMMAND git log -1 --format=%H
    WORKING_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}
    OUTPUT_VARIABLE ALPAQA_COMMIT_HASH
    OUTPUT_STRIP_TRAILING_WHITESPACE
    ERROR_QUIET)
configure_file(${CMAKE_CURRENT_LIST_DIR}/alpaqa-build-time.cpp.in
    alpaqa-build-time.cpp @ONLY)
