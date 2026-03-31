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

# Installation
if (ALPAQA_STANDALONE)
    set_target_properties(alpaqa-jl PROPERTIES INSTALL_RPATH "$ORIGIN")
endif()
install(TARGETS alpaqa-jl
    RUNTIME DESTINATION "${ALPAQA_INSTALL_BINDIR}"
        COMPONENT julia
    LIBRARY DESTINATION "${ALPAQA_INSTALL_LIBDIR}"
        COMPONENT julia)
