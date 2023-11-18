include(${PROJECT_SOURCE_DIR}/cmake/Debug.cmake)

# Set the runtime linker/loader search paths to make alpaqa stand-alone
if (ALPAQA_STANDALONE)
    cmake_path(RELATIVE_PATH ALPAQA_INSTALL_LIBDIR
               BASE_DIRECTORY ALPAQA_INSTALL_BINDIR
               OUTPUT_VARIABLE ALPAQA_INSTALL_LIBRELBINDIR)
    foreach (TGT IN LISTS ALPAQA_INSTALL_TARGETS ALPAQA_INSTALL_EXE)
        set_target_properties(${TGT} PROPERTIES
            INSTALL_RPATH "$ORIGIN;$ORIGIN/${ALPAQA_INSTALL_LIBRELBINDIR}")
    endforeach()
endif()

# Install everything
message(STATUS "Targets to install: ${ALPAQA_INSTALL_TARGETS};${ALPAQA_INSTALL_EXE}")
set(ALPAQA_COMPONENT_alpaqa "lib")
set(ALPAQA_COMPONENT_casadi-loader "casadi")
set(ALPAQA_COMPONENT_casadi-ocp-loader "casadi")
set(ALPAQA_COMPONENT_dl-api "dl_dev")
set(ALPAQA_COMPONENT_dl-loader "dl")
set(ALPAQA_COMPONENT_cutest-interface "extra")
set(ALPAQA_COMPONENT_ipopt-adapter "extra")
set(ALPAQA_COMPONENT_lbfgsb-fortran "extra")
set(ALPAQA_COMPONENT_lbfgsb-adapter "extra")
set(ALPAQA_COMPONENT_qpalm-adapter "extra")
set(ALPAQA_COMPONENT_driver "bin")
set(ALPAQA_COMPONENT_gradient-checker "bin")
# set(ALPAQA_OPTIONAL_COMPONENTS "lib;casadi;dl_dev;dl;extra;bin")
# set(ALPAQA_COMPONENT_casadi_DEPENDS "lib")
# set(ALPAQA_COMPONENT_dl_DEPENDS "lib;dl_dev")
# set(ALPAQA_COMPONENT_extra_DEPENDS "lib")
# set(ALPAQA_COMPONENT_bin_DEPENDS "lib")

# Install the dummy targets
install(TARGETS warnings
        EXPORT alpaqa-Targets)
# Install the actual targets
foreach(TGT IN LISTS ALPAQA_INSTALL_TARGETS ALPAQA_INSTALL_EXE)
    set(COMP "${ALPAQA_COMPONENT_${TGT}}")
    if (NOT COMP)
        message(FATAL_ERROR "Unknown component for target ${TGT}")
    endif()
    # Install the target files
    install(TARGETS ${TGT}
        EXPORT alpaqa-${COMP}Targets
        RUNTIME DESTINATION "${ALPAQA_INSTALL_BINDIR}"
            COMPONENT ${COMP}
        LIBRARY DESTINATION "${ALPAQA_INSTALL_LIBDIR}"
            COMPONENT ${COMP}
            NAMELINK_COMPONENT dev
        ARCHIVE DESTINATION "${ALPAQA_INSTALL_LIBDIR}"
            COMPONENT dev)
    list(APPEND ALPAQA_ALL_COMPONENTS ${COMP})
endforeach()
list(REMOVE_DUPLICATES ALPAQA_ALL_COMPONENTS)
list(APPEND ALPAQA_ALL_COMPONENTS "")
foreach(COMP IN LISTS ALPAQA_ALL_COMPONENTS)
    # Install the target CMake definitions
    install(EXPORT alpaqa-${COMP}Targets
        FILE alpaqa-${COMP}Targets.cmake
        DESTINATION "${ALPAQA_INSTALL_CMAKEDIR}"
            COMPONENT dev
        NAMESPACE alpaqa::)
    # Add all targets to the build tree export set
    export(EXPORT alpaqa-${COMP}Targets
        FILE "${PROJECT_BINARY_DIR}/alpaqa-${COMP}Targets.cmake"
        NAMESPACE alpaqa::)
endforeach()

# Install the header files
set(ALPAQA_INCLUDE_DIRS
    "${PROJECT_BINARY_DIR}/include/"
    "${PROJECT_SOURCE_DIR}/src/alpaqa/include/"
    "${PROJECT_SOURCE_DIR}/src/interop/casadi/include/"
    "${PROJECT_SOURCE_DIR}/src/interop/ipopt/include/"
    "${PROJECT_SOURCE_DIR}/src/interop/lbfgsb/include/"
    "${PROJECT_SOURCE_DIR}/src/interop/dl/include/"
    "${CMAKE_CURRENT_BINARY_DIR}/export/alpaqa"
)
foreach(DIR IN LISTS ALPAQA_INCLUDE_DIRS)
    install(DIRECTORY ${DIR}
        DESTINATION "${ALPAQA_INSTALL_INCLUDEDIR}"
            COMPONENT dev
        FILES_MATCHING REGEX "/.*\\.(h|[hti]pp)$")
endforeach()
install(DIRECTORY "${PROJECT_SOURCE_DIR}/src/interop/dl-api/include/"
    DESTINATION "${ALPAQA_INSTALL_INCLUDEDIR}"
        COMPONENT dl_dev
    FILES_MATCHING REGEX "/.*\\.(h|[hti]pp)$")

# Install the debug files
foreach(target IN LISTS ALPAQA_INSTALL_TARGETS ALPAQA_INSTALL_EXE)
    get_target_property(target_type ${target} TYPE)
    if (${target_type} STREQUAL "SHARED_LIBRARY")
        alpaqa_install_debug_syms(${target} debug
                                  ${ALPAQA_INSTALL_LIBDIR}
                                  ${ALPAQA_INSTALL_BINDIR})
    elseif (${target_type} STREQUAL "EXECUTABLE")
        alpaqa_install_debug_syms(${target} debug
                                  ${ALPAQA_INSTALL_BINDIR}
                                  ${ALPAQA_INSTALL_BINDIR})
    endif()
endforeach()

# Generate the config file that includes the exports
include(CMakePackageConfigHelpers)
configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/Config.cmake.in"
    "${PROJECT_BINARY_DIR}/alpaqaConfig.cmake"
    INSTALL_DESTINATION "${ALPAQA_INSTALL_CMAKEDIR}"
    NO_SET_AND_CHECK_MACRO)
write_basic_package_version_file(
    "${PROJECT_BINARY_DIR}/alpaqaConfigVersion.cmake"
    VERSION "${PROJECT_VERSION}"
    COMPATIBILITY SameMajorVersion)

# Install the alpaqaConfig.cmake and alpaqaConfigVersion.cmake
install(FILES
    "${PROJECT_BINARY_DIR}/alpaqaConfig.cmake"
    "${PROJECT_BINARY_DIR}/alpaqaConfigVersion.cmake"
    DESTINATION "${ALPAQA_INSTALL_CMAKEDIR}"
        COMPONENT dev)
