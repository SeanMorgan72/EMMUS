
# ============================================================================
# EMMUS CLI Application
# ============================================================================

set(EMMUS_CLI_SOURCES

    "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp"

)

# ============================================================================
# Validate Entry Point
# ============================================================================

if(NOT EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/main.cpp")

    message(FATAL_ERROR
        "EMMUS_BUILD_CLI=ON, but apps/emmus-cli/main.cpp does not exist."
    )

endif()

# ============================================================================
# Executable
# ============================================================================

add_executable(
    emmus-cli

    ${EMMUS_CLI_SOURCES}
)

# Namespaced target.
add_executable(
    EMMUS::CLI
    ALIAS emmus-cli
)

# ============================================================================
# Dependencies
# ============================================================================

target_link_libraries(
    emmus-cli

    PRIVATE

        EMMUS::Core
)

# ============================================================================
# Language
# ============================================================================

target_compile_features(
    emmus-cli

    PRIVATE

        cxx_std_23
)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(
    emmus-cli

    PRIVATE

        "${PROJECT_SOURCE_DIR}/include"
)

# ============================================================================
# Output
# ============================================================================

set_target_properties(
    emmus-cli

    PROPERTIES

        OUTPUT_NAME emmus
)

# ============================================================================
# Compiler Warnings
# ============================================================================

if(EMMUS_ENABLE_WARNINGS)

    emmuss_enable_warnings(emmus-cli)

endif()

# ============================================================================
# Sanitizers
# ============================================================================

if(EMMUS_ENABLE_SANITIZERS)

    emmuss_enable_sanitizers(emmus-cli)

endif()
