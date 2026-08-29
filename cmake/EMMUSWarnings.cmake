
# ============================================================================
# EMMUS Compiler Warnings
# ============================================================================

function(emmuss_enable_warnings target)

    if(MSVC)

        target_compile_options(

            ${target}

            PRIVATE

                /W4
                /permissive-
        )

    elseif(CMAKE_CXX_COMPILER_ID MATCHES
           "GNU|Clang|AppleClang")

        target_compile_options(

            ${target}

            PRIVATE

                -Wall
                -Wextra
                -Wpedantic
                -Wconversion
                -Wsign-conversion
                -Wshadow
                -Wformat=2
                -Wundef
                -Werror=return-type
        )

    endif()

endfunction()
