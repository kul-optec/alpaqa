cmake_minimum_required(VERSION 3.21...4.1)
project(alpaqa-examples C CXX)
include(CTest)

if (PROJECT_IS_TOP_LEVEL)
    find_package(alpaqa REQUIRED CONFIG OPTIONAL_COMPONENTS CasADi Dl DlLoader Extra CUTEst)
endif()

include(GNUInstallDirs)

function(alpaqa_install_example TGT)
    install(TARGETS ${TGT}
        RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
            COMPONENT extra
        LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT extra
            NAMELINK_COMPONENT extra_dev
        ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
            COMPONENT extra_dev)
endfunction()

add_subdirectory(util)

add_custom_target(examples ALL)
function(alpaqa_register_example TGT)
    target_link_libraries(${TGT} PRIVATE alpaqa::example-util)
    add_dependencies(examples ${TGT})
    alpaqa_install_example(${TGT})
endfunction()

add_subdirectory(C++)
add_subdirectory(problems)
