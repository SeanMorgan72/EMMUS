
# ============================================================================
# EMMUS GUI Application
# ============================================================================
#
# The GUI is intentionally optional.
#
# The EMMUS architecture requires the core simulation engine to remain
# independently buildable and testable. GUI dependencies must therefore
# remain outside EMMUS::Core.
#
# The GUI technology and implementation will be added here once the GUI
# source tree has been established.
# ============================================================================

message(FATAL_ERROR

    "EMMUS_BUILD_GUI=ON, but the GUI implementation has not yet been "
    "established under apps/emmus-gui/."
    "\n\n"
    "Keep EMMUS_BUILD_GUI=OFF until the GUI implementation and its "
    "dependencies have been added."

)
