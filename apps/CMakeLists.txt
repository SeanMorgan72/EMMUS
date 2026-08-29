
# ============================================================================
# EMMUS Applications
# ============================================================================

if(EMMUS_BUILD_CLI)

    add_subdirectory(emmus-cli)

endif()

if(EMMUS_BUILD_GUI)

    add_subdirectory(emmus-gui)

endif()
