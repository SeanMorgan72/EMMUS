
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

        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )

    FetchContent_MakeAvailable(googletest)

endif()

# ============================================================================
# GoogleTest CMake Integration
# ============================================================================

include(GoogleTest)

# ============================================================================
# Test Suite Helper
# ============================================================================

function(emmuss_add_test_suite target)

    cmake_parse_arguments(

        TEST

        ""

        ""

        "SOURCES"

        ${ARGN}
    )

    if(NOT TEST_SOURCES)

        message(FATAL_ERROR
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
    # Dependencies
    # ------------------------------------------------------------------------

    target_link_libraries(

        ${target}

        PRIVATE

            EMMUS::Core

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

        emmuss_enable_warnings(${target})

    endif()

    # ------------------------------------------------------------------------
    # Sanitizers
    # ------------------------------------------------------------------------

    if(EMMUS_ENABLE_SANITIZERS)

        emmuss_enable_sanitizers(${target})

    endif()

    # ------------------------------------------------------------------------
    # Automatic GoogleTest Discovery
    # ------------------------------------------------------------------------

    gtest_discover_tests(

        ${target}

        DISCOVERY_MODE PRE_TEST

        WORKING_DIRECTORY
            "${PROJECT_BINARY_DIR}"
    )

endfunction()

# ============================================================================
# Unit Tests
# ============================================================================

file(GLOB_RECURSE EMMUS_UNIT_TEST_SOURCES

    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/unit/*.cxx"
)

if(EMMUS_UNIT_TEST_SOURCES)

    emmuss_add_test_suite(

        emmus-unit-tests

        SOURCES
            ${EMMUS_UNIT_TEST_SOURCES}
    )

else()

    message(STATUS
        "No unit-test sources found."
    )

endif()

# ============================================================================
# Integration Tests
# ============================================================================

file(GLOB_RECURSE EMMUS_INTEGRATION_TEST_SOURCES

    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/integration/*.cxx"
)

if(EMMUS_INTEGRATION_TEST_SOURCES)

    emmuss_add_test_suite(

        emmus-integration-tests

        SOURCES
            ${EMMUS_INTEGRATION_TEST_SOURCES}
    )

else()

    message(STATUS
        "No integration-test sources found."
    )

endif()

# ============================================================================
# System Tests
# ============================================================================

file(GLOB_RECURSE EMMUS_SYSTEM_TEST_SOURCES

    CONFIGURE_DEPENDS

    "${CMAKE_CURRENT_SOURCE_DIR}/system/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/system/*.cc"
    "${CMAKE_CURRENT_SOURCE_DIR}/system/*.cxx"
)

if(EMMUS_SYSTEM_TEST_SOURCES)

    emmuss_add_test_suite(

        emmus-system-tests

        SOURCES
            ${EMMUS_SYSTEM_TEST_SOURCES}
    )

else()

    message(STATUS
        "No system-test sources found."
    )

endif()
