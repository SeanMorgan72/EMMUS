add_executable(
    emmus-demo
    main.cpp
)

add_executable(
    EMMUS::Demo
    ALIAS emmus-demo
)

target_link_libraries(
    emmus-demo
    PRIVATE
        EMMUS::Core
)

target_compile_features(
    emmus-demo
    PRIVATE
        cxx_std_23
)

if(EMMUS_ENABLE_WARNINGS)
    emmuss_enable_warnings(emmus-demo)
endif()

if(EMMUS_ENABLE_SANITIZERS)
    emmuss_enable_sanitizers(emmus-demo)
endif()
