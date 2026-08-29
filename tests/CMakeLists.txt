# ============================================================================
# EMMUS Test Infrastructure
# ============================================================================

include(FetchContent)

# ============================================================================
# GoogleTest
# ============================================================================
#
# First attempt to use an installed GoogleTest package.
#
# If GoogleTest is not installed, FetchContent obtains a known version.
# This keeps the build reproducible on developer and CI machines.
# ============================================================================

find_package(
    GTest
    CONFIG
    QUIET
)

if(NOT GTest_FOUND)

    message(STATUS
        "GoogleTest package not found."
    )

    message(STATUS
        "Fetching GoogleTest 1.17.0..."
    )

    # Do not build GoogleTest's own tests.
    set(
        INSTALL_GTEST
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    set(
        BUILD_GMOCK
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    set(
        gtest_build_tests
        OFF
        CACHE BOOL
        ""
        FORCE
    )

    # Required when building with MSVC.
    set(
        gtest_force_shared_crt
        ON
        CACHE BOOL
        ""
        FORCE
    )

    FetchContent_Declare(

        googletest

        URL
            https://github.com/google/googletest/archive/refs/tags/v1.17.0.zip

        DOWNLOAD_EXTRACT_TIMESTAMP
            TRUE
    )

    FetchContent_MakeAvailable(
        googletest
    )

endif()


# ============================================================================
# GoogleTest CMake Integration
# ============================================================================

include(GoogleTest)


# ============================================================================
# Test Infrastructure Helper
# ============================================================================
#
# Creates a complete EMMUS GoogleTest executable and automatically registers
# all GoogleTest cases with CTest.
#
# Usage:
#
#     emmuss_add_test_suite(
#         emmus-unit-tests
#
#         LABEL unit
#
#         SOURCES
#             ...
#     )
#
# ============================================================================

function(emmuss_add_test_suite target)

    cmake_parse_arguments(

        TEST

        ""

        "LABEL;WORKING_DIRECTORY"

        "SOURCES"

        ${ARGN}
    )


    # ------------------------------------------------------------------------
    # Validate Sources
    # ------------------------------------------------------------------------

    if(NOT TEST_SOURCES)

        message(
            FATAL_ERROR
            "No test sources were supplied for ${target}."
        )

    endif()


    # ------------------------------------------------------------------------
    # Test Executable
    # ------------------------------------------------------------------------

    add_executable(

        ${target}

        ${TEST_SOURCES}
    )


    # ------------------------------------------------------------------------
    # EMMUS Core + GoogleTest
    # ------------------------------------------------------------------------

    target_link_libraries(

        ${target}

        PRIVATE

            #EMMUS::Core

            GTest::gtest_main
    )


    # ------------------------------------------------------------------------
    # C++ Standard
    # ------------------------------------------------------------------------

    target_compile_features(

        ${target}

        PRIVATE

            cxx_std_23
    )


    # ------------------------------------------------------------------------
    # Include Directories
    # ------------------------------------------------------------------------

    target_include_directories(

        ${target}

        PRIVATE

            "${PROJECT_SOURCE_DIR}/include"

            "${PROJECT_SOURCE_DIR}/tests"
    )


    # ------------------------------------------------------------------------
    # Compiler Warnings
    # ------------------------------------------------------------------------

    if(EMMUS_ENABLE_WARNINGS)

        emmuss_enable_warnings(
            ${target}
        )

    endif()


    # ------------------------------------------------------------------------
    # Sanitizers
    # ------------------------------------------------------------------------

    if(EMMUS_ENABLE_SANITIZERS)

        emmuss_enable_sanitizers(
            ${target}
        )

    endif()


    # ------------------------------------------------------------------------
    # Test Working Directory
    # ------------------------------------------------------------------------

    if(TEST_WORKING_DIRECTORY)

        set(
            _test_working_directory
            "${TEST_WORKING_DIRECTORY}"
        )

    else()

        set(
            _test_working_directory
            "${PROJECT_BINARY_DIR}"
        )

    endif()


    # ------------------------------------------------------------------------
    # Automatic GoogleTest Discovery
    # ------------------------------------------------------------------------
    #
    # GoogleTest's TEST(), TEST_F(), and TEST_P() cases are discovered
    # automatically and registered with CTest.
    #
    # POST_BUILD is preferable here because discovery happens after the test
    # executable has been built.
    #
    # ------------------------------------------------------------------------

    gtest_discover_tests(

        ${target}

        DISCOVERY_MODE
            POST_BUILD

        DISCOVERY_TIMEOUT
            30

        WORKING_DIRECTORY
            "${_test_working_directory}"

        PROPERTIES

            LABELS
                "${TEST_LABEL}"
    )

endfunction()


# ============================================================================
# Unit Tests
# ============================================================================
#
# Unit tests verify individual EMMUS components in isolation.
#
# Expected areas include:
#
#   algorithms/
#   application/
#   infrastructure/
#   memory/
#   simulation/
#   statistics/
#
# ============================================================================

file(
    GLOB_RECURSE
    EMMUS_UNIT_TEST_SOURCES

    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cxx"
)


if(EMMUS_UNIT_TEST_SOURCES)

    emmuss_add_test_suite(

        emmus-unit-tests

        LABEL
            unit

        SOURCES
            ${EMMUS_UNIT_TEST_SOURCES}
    )

else()

    message(
        STATUS
        "No unit-test sources found."
    )

endif()


# ============================================================================
# Integration Tests
# ============================================================================
#
# Integration tests verify interactions between EMMUS subsystems.
#
# Examples include:
#
#   MMU + PageTable
#   MMU + PhysicalMemory
#   MMU + ReplacementPolicy
#   Simulation + Statistics
#
# ============================================================================

file(
    GLOB_RECURSE
    EMMUS_INTEGRATION_TEST_SOURCES

    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cxx"
)


if(EMMUS_INTEGRATION_TEST_SOURCES)

    emmuss_add_test_suite(

        emmus-integration-tests

        LABEL
            integration

        SOURCES
            ${EMMUS_INTEGRATION_TEST_SOURCES}
    )

else()

    message(
        STATUS
        "No integration-test sources found."
    )

endif()


# ============================================================================
# System Tests
# ============================================================================
#
# System tests exercise complete EMMUS simulation scenarios.
#
# These tests should remain independent from GUI presentation components.
#
# ============================================================================

file(
    GLOB_RECURSE
    EMMUS_SYSTEM_TEST_SOURCES

    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/system/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/system/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/system/*.cxx"
)


if(EMMUS_SYSTEM_TEST_SOURCES)

    emmuss_add_test_suite(

        emmus-system-tests

        LABEL
            system

        SOURCES
            ${EMMUS_SYSTEM_TEST_SOURCES}
    )

else()

    message(
        STATUS
        "No system-test sources found."
    )

endif()


# ============================================================================
# Aggregate EMMUS Test Target
# ============================================================================
#
# This target provides a convenient way to build every available EMMUS
# test executable.
#
# It does not execute the tests. Use CTest for execution.
#
# ============================================================================

add_custom_target(
    emmus-tests
)


if(TARGET emmus-unit-tests)

    add_dependencies(
        emmus-tests
        emmus-unit-tests
    )

endif()


if(TARGET emmus-integration-tests)

    add_dependencies(
        emmus-tests
        emmus-integration-tests
    )

endif()


if(TARGET emmus-system-tests)

    add_dependencies(
        emmus-tests
        emmus-system-tests
    )

endif()