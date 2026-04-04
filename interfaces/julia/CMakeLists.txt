add_library(alpaqa-jl MODULE "alm.jl.cpp")
target_link_libraries(alpaqa-jl
    PUBLIC alpaqa::alpaqa alpaqa::problem-loader alpaqa::drivers)
set_target_properties(alpaqa-jl PROPERTIES PREFIX "")
include(GenerateExportHeader)
generate_export_header(alpaqa-jl
    BASE_NAME alpaqa_jl
    EXPORT_FILE_NAME export-jl/alpaqa-jl/export.h)
target_include_directories(alpaqa-jl PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export-jl>)
alpaqa_configure_visibility(alpaqa-jl)

# Set the runtime linker/loader search paths to make alpaqa stand-alone
if (ALPAQA_STANDALONE)
    if (APPLE)
        set_target_properties(alpaqa-jl PROPERTIES INSTALL_RPATH "$@loader_path")
    else()
        set_target_properties(alpaqa-jl PROPERTIES INSTALL_RPATH "$ORIGIN")
    endif()
endif()

# Install the Julia interface
install(TARGETS alpaqa-jl
    EXPORT alpaqaJuliaTargets
    RUNTIME DESTINATION "${ALPAQA_INSTALL_BINDIR}"
        COMPONENT julia
    LIBRARY DESTINATION "${ALPAQA_INSTALL_LIBDIR}"
        COMPONENT julia
        NAMELINK_COMPONENT julia_dev
    ARCHIVE DESTINATION "${ALPAQA_INSTALL_LIBDIR}"
        COMPONENT julia_dev)
alpaqa_install_config(Julia julia_dev)
