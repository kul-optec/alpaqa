function(configure_problem_visibility target)
    set_target_properties(${target} PROPERTIES CXX_VISIBILITY_PRESET "hidden"
                                               C_VISIBILITY_PRESET "hidden"
                                               VISIBILITY_INLINES_HIDDEN true)
    if (CMAKE_SYSTEM_NAME MATCHES "Linux")
        set(VERSION_SCRIPT "${CMAKE_CURRENT_BINARY_DIR}/${target}-export.lds")
        file(WRITE ${VERSION_SCRIPT}
            "{ global: alpaqa_problem_register; local: *; };")
        target_link_options(${target} PRIVATE
            "LINKER:--version-script=${VERSION_SCRIPT}")
    endif()
endfunction()

function(add_problem_module name)
    add_library(${name} MODULE "${name}.cpp")
    target_link_libraries(${name} PRIVATE alpaqa::dl-api alpaqa::alpaqa)
    configure_problem_visibility(${name})
    set_target_properties(${name} PROPERTIES
        PREFIX "" RELEASE_POSTFIX "" DEBUG_POSTFIX "" RELWITHDEBINFO_POSTFIX ""
        MINSIZEREL_POSTFIX "")
    include(GenerateExportHeader)
    generate_export_header(${name}
        EXPORT_FILE_NAME export/${name}-export.h)
    target_include_directories(${name} PRIVATE
        $<BUILD_INTERFACE:${CMAKE_CURRENT_BINARY_DIR}/export>)
    target_compile_features(${name} PRIVATE cxx_std_20)
endfunction()

if (TARGET alpaqa::dl-api)
    add_problem_module("sparse-logistic-regression")
endif()
