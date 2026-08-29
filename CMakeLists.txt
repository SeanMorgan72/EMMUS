
cmake_minimum_required(VERSION 3.24)

project(EMMUS
    VERSION 2.0.0
    DESCRIPTION "Enhanced Memory Management Unit Simulator"
    LANGUAGES CXX
)

# ============================================================================
# Project Options
# ============================================================================

option(EMMUS_BUILD_TESTS
    "Build unit, integration, and system tests"
    ON
)

option(EMMUS_BUILD_CLI
    "Build the EMMUS command-line application"
    OFF
)

option(EMMUS_BUILD_GUI
    "Build the EMMUS graphical application"
    OFF
)

option(EMMUS_ENABLE_WARNINGS
    "Enable project compiler warnings"
    ON
)

option(EMMUS_ENABLE_SANITIZERS
    "Enable AddressSanitizer and UndefinedBehaviorSanitizer"
    OFF
)

option(EMMUS_ENABLE_INSTALL
    "Enable installation rules"
    ON
)

# ============================================================================
# C++ Standard
# ============================================================================

set(
    EMMUS_CXX_STANDARD
    23
    CACHE STRING
    "C++ standard required by EMMUS"
)

set(
    CMAKE_CXX_STANDARD
    ${EMMUS_CXX_STANDARD}
)

set(
    CMAKE_CXX_STANDARD_REQUIRED
    ON
)

set(
    CMAKE_CXX_EXTENSIONS
    OFF
)

# ============================================================================
# Build Output Directories
# ============================================================================

set(
    CMAKE_ARCHIVE_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/lib"
)

set(
    CMAKE_LIBRARY_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/lib"
)

set(
    CMAKE_RUNTIME_OUTPUT_DIRECTORY
    "${CMAKE_BINARY_DIR}/bin"
)

# ============================================================================
# Project CMake Modules
# ============================================================================
#
# This tells CMake to search:
#
#   ${CMAKE_CURRENT_SOURCE_DIR}/cmake/
#
# when resolving custom include() commands such as:
#
#   include(EMMUSWarnings)
#   include(EMMUSSanitizers)
#   include(EMMUSInstall)
#
# ============================================================================

list(
    APPEND
    CMAKE_MODULE_PATH
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake"
)

# Standard CMake installation directory variables:
#
#   CMAKE_INSTALL_BINDIR
#   CMAKE_INSTALL_LIBDIR
#   CMAKE_INSTALL_INCLUDEDIR
#
include(GNUInstallDirs)

# ============================================================================
# EMMUS CMake Modules
# ============================================================================

include(EMMUSWarnings)

include(EMMUSSanitizers)

# ============================================================================
# Build Type
# ============================================================================

if(
    NOT CMAKE_CONFIGURATION_TYPES
    AND NOT CMAKE_BUILD_TYPE
)

    set(
        CMAKE_BUILD_TYPE
        RelWithDebInfo
        CACHE STRING
        "Build type"
        FORCE
    )

endif()

set_property(
    CACHE CMAKE_BUILD_TYPE
    PROPERTY STRINGS
        Debug
        Release
        RelWithDebInfo
        MinSizeRel
)

# ============================================================================
# Core Library
# ============================================================================

add_subdirectory(
    src
)

# ============================================================================
# Applications
# ============================================================================

if(
    EMMUS_BUILD_CLI
    OR EMMUS_BUILD_GUI
)

    add_subdirectory(
        apps
    )

endif()

# ============================================================================
# Testing
# ============================================================================

include(CTest)

if(
    BUILD_TESTING
    AND EMMUS_BUILD_TESTS
)

    add_subdirectory(
        tests
    )

endif()

# ============================================================================
# Installation
# ============================================================================

if(EMMUS_ENABLE_INSTALL)

    include(EMMUSInstall)

endif()

# ============================================================================
# Configuration Summary
# ============================================================================

message(STATUS "")

message(STATUS "============================================================")
message(STATUS "EMMUS ${PROJECT_VERSION}")
message(STATUS "============================================================")

message(STATUS
    "C++ Standard       : C++${EMMUS_CXX_STANDARD}"
)

message(STATUS
    "Build Type         : ${CMAKE_BUILD_TYPE}"
)

message(STATUS
    "Build Tests        : ${EMMUS_BUILD_TESTS}"
)

message(STATUS
    "Build CLI          : ${EMMUS_BUILD_CLI}"
)

message(STATUS
    "Build GUI          : ${EMMUS_BUILD_GUI}"
)

message(STATUS
    "Compiler Warnings   : ${EMMUS_ENABLE_WARNINGS}"
)

message(STATUS
    "Sanitizers          : ${EMMUS_ENABLE_SANITIZERS}"
)

message(STATUS
    "Installation        : ${EMMUS_ENABLE_INSTALL}"
)

message(STATUS
    "Generator           : ${CMAKE_GENERATOR}"
)

message(STATUS
    "C++ Compiler        : ${CMAKE_CXX_COMPILER}"
)

message(STATUS "============================================================")

message(STATUS "")
