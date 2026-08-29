
# ============================================================================
# EMMUS Installation
# ============================================================================
#
# Installation rules are conditional on targets existing. This allows the
# repository to configure successfully while the implementation is being
# developed incrementally.
# ============================================================================

include(CMakePackageConfigHelpers)

# ============================================================================
# Public Headers
# ============================================================================
#
# Install the public EMMUS header hierarchy:
#
#   include/emmus/...
#
# ============================================================================

if(EXISTS "${PROJECT_SOURCE_DIR}/include")

    install(
        DIRECTORY
            "${PROJECT_SOURCE_DIR}/include/"
        DESTINATION
            "${CMAKE_INSTALL_INCLUDEDIR}"
    )

endif()

# ============================================================================
# Core Library
# ============================================================================

if(TARGET emmus_core)

    install(
        TARGETS
            emmus_core

        EXPORT
            EMMUSTargets

        ARCHIVE DESTINATION
            "${CMAKE_INSTALL_LIBDIR}"

        LIBRARY DESTINATION
            "${CMAKE_INSTALL_LIBDIR}"

        RUNTIME DESTINATION
            "${CMAKE_INSTALL_BINDIR}"

        INCLUDES DESTINATION
            "${CMAKE_INSTALL_INCLUDEDIR}"
    )

endif()

# ============================================================================
# CLI Application
# ============================================================================

if(TARGET emmus-cli)

    install(
        TARGETS
            emmus-cli

        RUNTIME DESTINATION
            "${CMAKE_INSTALL_BINDIR}"
    )

endif()

# ============================================================================
# GUI Application
# ============================================================================

if(TARGET emmus-gui)

    install(
        TARGETS
            emmus-gui

        RUNTIME DESTINATION
            "${CMAKE_INSTALL_BINDIR}"
    )

endif()

# ============================================================================
# CMake Target Export
# ============================================================================
#
# Only create the export when at least one exported target exists.
#
# ============================================================================

if(TARGET emmus_core)

    install(
        EXPORT
            EMMUSTargets

        FILE
            EMMUSTargets.cmake

        NAMESPACE
            EMMUS::

        DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/EMMUS"
    )

    # ------------------------------------------------------------------------
    # Package Configuration
    # ------------------------------------------------------------------------

    configure_package_config_file(

        "${PROJECT_SOURCE_DIR}/cmake/EMMUSConfig.cmake.in"

        "${CMAKE_CURRENT_BINARY_DIR}/EMMUSConfig.cmake"

        INSTALL_DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/EMMUS"
    )

    # ------------------------------------------------------------------------
    # Package Version
    # ------------------------------------------------------------------------

    write_basic_package_version_file(

        "${CMAKE_CURRENT_BINARY_DIR}/EMMUSConfigVersion.cmake"

        VERSION
            "${PROJECT_VERSION}"

        COMPATIBILITY
            SameMajorVersion
    )

    # ------------------------------------------------------------------------
    # Install Package Configuration
    # ------------------------------------------------------------------------

    install(

        FILES

            "${CMAKE_CURRENT_BINARY_DIR}/EMMUSConfig.cmake"

            "${CMAKE_CURRENT_BINARY_DIR}/EMMUSConfigVersion.cmake"

        DESTINATION
            "${CMAKE_INSTALL_LIBDIR}/cmake/EMMUS"
    )

else()

    message(STATUS
        "EMMUS::Core does not exist yet; library installation/export "
        "rules will be enabled automatically when the core is created."
    )

endif()
