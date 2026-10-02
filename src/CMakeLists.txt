
# ============================================================================
# EMMUS Core Library
# ============================================================================
#
# Production source layout:
#
#   src/emmus/
#       algorithms/
#           replacement/
#       application/
#       infrastructure/
#           configuration/
#           logging/
#           timing/
#       memory/
#           mmu/
#           virtual/
#           physical/
#       simulation/
#       statistics/
#
# Public headers are located under:
#
#   include/emmus/
#
# ============================================================================

# ----------------------------------------------------------------------------
# Discover Production Sources
# ----------------------------------------------------------------------------

file(GLOB_RECURSE EMMUS_CORE_SOURCES
    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/emmus/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/emmus/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/emmus/*.cxx"
)

# ----------------------------------------------------------------------------
# Discover Public Headers
# ----------------------------------------------------------------------------

file(GLOB_RECURSE EMMUS_CORE_HEADERS
    CONFIGURE_DEPENDS

    "${PROJECT_SOURCE_DIR}/include/emmus/*.hpp"
    "${PROJECT_SOURCE_DIR}/include/emmus/*.h"
    "${PROJECT_SOURCE_DIR}/include/emmus/*.hh"
    "${PROJECT_SOURCE_DIR}/include/emmus/*.hxx"
)

if(NOT EMMUS_CORE_SOURCES)
    message(FATAL_ERROR
        "No EMMUS core sources found under ${CMAKE_CURRENT_SOURCE_DIR}/emmus. "
        "The core library is required by the configured applications and tests."
    )
endif()

# ============================================================================
# EMMUS Core Static Library
# ============================================================================

add_library(
    emmus_core
    STATIC

    ${EMMUS_CORE_SOURCES}
    ${EMMUS_CORE_HEADERS}
)

# Public namespaced target.
add_library(
    EMMUS::Core
    ALIAS
    emmus_core
)

# ============================================================================
# C++23
# ============================================================================

target_compile_features(
    emmus_core
    PUBLIC

        cxx_std_23
)

# ============================================================================
# Include Directories
# ============================================================================

target_include_directories(
    emmus_core

    PUBLIC

        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>

        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

# ============================================================================
# Library Properties
# ============================================================================

set_target_properties(
    emmus_core

    PROPERTIES

        OUTPUT_NAME emmus_core
        EXPORT_NAME Core

        POSITION_INDEPENDENT_CODE ON
)

# ============================================================================
# Compiler Warnings
# ============================================================================

if(EMMUS_ENABLE_WARNINGS)

    emmuss_enable_warnings(
        emmus_core
    )

endif()

# ============================================================================
# Sanitizers
# ============================================================================

if(EMMUS_ENABLE_SANITIZERS)

    emmuss_enable_sanitizers(
        emmus_core
    )

endif()

# ============================================================================
# Diagnostic Information
# ============================================================================

message(STATUS
    "EMMUS core library configured."
)

message(STATUS
    "EMMUS core source count: ${EMMUS_CORE_SOURCES}"
)
