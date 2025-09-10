set(ALPAQA_EXAMPLE_TARGETS)

if (TARGET alpaqa::dl-api)
    alpaqa_add_dl_problem_module(sparse-logistic-regression LINK_ALPAQA)
    list(APPEND ALPAQA_EXAMPLE_TARGETS sparse-logistic-regression)
endif()

# TODO: move to a more suitable location?
if (ALPAQA_WITH_PYTHON_PROBLEM_LOADER AND NOT TARGET alpaqa::dl-api)
    message(SEND_ERROR
        "ALPAQA_WITH_PYTHON_PROBLEM_LOADER requires ALPAQA_WITH_DL")
elseif (ALPAQA_WITH_PYTHON_PROBLEM_LOADER)
    if (CMAKE_CROSSCOMPILING AND NOT DEFINED CMAKE_CROSSCOMPILING_EMULATOR)
        find_package(Python3 REQUIRED COMPONENTS Development)
    else()
        find_package(Python3 REQUIRED COMPONENTS Interpreter Development)
    endif()
    include(../../interfaces/python/cmake/QueryPythonForPybind11.cmake)
    find_pybind11_python_first()
    alpaqa_add_dl_problem_module("python-loader" LINK_ALPAQA VISIBILITY_DEFAULT)
    target_link_libraries(python-loader PRIVATE pybind11::embed)
    list(APPEND ALPAQA_EXAMPLE_TARGETS "python-loader")
endif()

include(GNUInstallDirs)
install(TARGETS ${ALPAQA_EXAMPLE_TARGETS}
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        COMPONENT extra
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT extra
        NAMELINK_COMPONENT extra_dev
    ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
        COMPONENT extra_dev)

if (BUILD_TESTING AND TARGET sparse-logistic-regression AND TARGET alpaqa::driver)
    add_test(NAME sparse-logistic-regression
             COMMAND alpaqa::driver
             dl:$<TARGET_FILE:sparse-logistic-regression>
             problem.datafile=${CMAKE_CURRENT_LIST_DIR}/breast_cancer.csv
             dl_flags.deepbind=false dl_flags.nodelete=false)
endif()
