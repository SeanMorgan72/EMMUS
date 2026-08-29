include_guard(GLOBAL)

include(GoogleTest)

# ---------------------------------------------------------------------------
# Configure an EMMUS GoogleTest target
# ---------------------------------------------------------------------------

function(emmus_configure_test_target target)

    cmake_parse_arguments(
        ARG
        ""
        "LABEL"
        ""
        ${ARGN}
    )

    if(NOT TARGET ${target})
        message(
            FATAL_ERROR
            "Unknown EMMUS test target: ${target}"
        )
    endif()

    target_compile_features(
        ${target}
        PRIVATE
            cxx_std_23
    )

    # -----------------------------------------------------------------------
    # Compiler warnings
    # -----------------------------------------------------------------------

    target_compile_options(
        ${target}
        PRIVATE

            $<$<CXX_COMPILER_ID:GNU,Clang>:
                -Wall
                -Wextra
                -Wpedantic
            >

            $<$<CXX_COMPILER_ID:MSVC>:
                /W4
            >
    )

    # -----------------------------------------------------------------------
    # Test labels
    # -----------------------------------------------------------------------

    if(ARG_LABEL)

        set(_labels
            "gtest;${ARG_LABEL}"
        )

    else()

        set(_labels
            "gtest"
        )

    endif()

    # -----------------------------------------------------------------------
    # Automatic GoogleTest discovery
    # -----------------------------------------------------------------------
    #
    # Every TEST(), TEST_F(), and TEST_P() case is discovered automatically
    # and registered with CTest.
    #

    gtest_discover_tests(
        ${target}

        DISCOVERY_MODE POST_BUILD

        DISCOVERY_TIMEOUT 30

        WORKING_DIRECTORY
            "${CMAKE_CURRENT_BINARY_DIR}"

        PROPERTIES
            LABELS "${_labels}"
    )

endfunction()


# ---------------------------------------------------------------------------
# Add a GoogleTest executable
# ---------------------------------------------------------------------------

function(emmus_add_gtest target)

    cmake_parse_arguments(
        ARG
        ""
        "WORKING_DIRECTORY;LABEL"
        "SOURCES;LIBRARIES;DEFINITIONS"
        ${ARGN}
    )

    # -----------------------------------------------------------------------
    # Test executable
    # -----------------------------------------------------------------------

    add_executable(
        ${target}
        ${ARG_SOURCES}
    )

    # -----------------------------------------------------------------------
    # Include directories
    # -----------------------------------------------------------------------

    target_include_directories(
        ${target}

        PRIVATE

            "${PROJECT_SOURCE_DIR}/include"

            "${PROJECT_SOURCE_DIR}/tests"
    )

    # -----------------------------------------------------------------------
    # Libraries
    # -----------------------------------------------------------------------

    target_link_libraries(
        ${target}

        PRIVATE

            GTest::gtest_main

            ${ARG_LIBRARIES}
    )

    # -----------------------------------------------------------------------
    # Optional compile definitions
    # -----------------------------------------------------------------------

    if(ARG_DEFINITIONS)

        target_compile_definitions(
            ${target}

            PRIVATE

                ${ARG_DEFINITIONS}
        )

    endif()

    # -----------------------------------------------------------------------
    # Configure test target
    # -----------------------------------------------------------------------

    emmus_configure_test_target(
        ${target}

        LABEL
            "${ARG_LABEL}"
    )

endfunction()