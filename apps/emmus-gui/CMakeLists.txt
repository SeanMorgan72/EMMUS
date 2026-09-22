
# ============================================================================
# EMMUS GUI Application
# ============================================================================
#
# The GUI remains optional and isolated from the core engine. This layer
# exposes the physical-memory utilization view and can be extended later to
# use a full windowing toolkit such as Qt without affecting EMMUS::Core.
# ============================================================================

set(EMMUS_GUI_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/PhysicalMemoryVisualizationView.cpp"
)

add_library(
    emmus_gui
    STATIC

    ${EMMUS_GUI_SOURCES}
)

add_library(
    EMMUS::GUI
    ALIAS
    emmus_gui
)

target_compile_features(
    emmus_gui
    PUBLIC
        cxx_std_23
)

target_include_directories(
    emmus_gui
    PUBLIC
        "${PROJECT_SOURCE_DIR}/include"
        "${PROJECT_SOURCE_DIR}/apps/emmus-gui"
)

target_link_libraries(
    emmus_gui
    PUBLIC
        EMMUS::Core
)

if(EMMUS_ENABLE_WARNINGS)
    emmuss_enable_warnings(emmus_gui)
endif()

if(EMMUS_ENABLE_SANITIZERS)
    emmuss_enable_sanitizers(emmus_gui)
endif()
