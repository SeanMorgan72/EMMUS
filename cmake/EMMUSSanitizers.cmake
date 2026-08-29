
# ============================================================================
# EMMUS Sanitizer Support
# ============================================================================

function(emmuss_enable_sanitizers target)

    if(MSVC)

        message(WARNING

            "EMMUS sanitizers are not configured for MSVC in this module."
        )

        return()

    endif()

    if(CMAKE_CXX_COMPILER_ID MATCHES
       "GNU|Clang|AppleClang")

        target_compile_options(

            ${target}

            PRIVATE

                -fsanitize=address,undefined
                -fno-omit-frame-pointer
        )

        target_link_options(

            ${target}

            PRIVATE

                -fsanitize=address,undefined
        )

    endif()

endfunction()
