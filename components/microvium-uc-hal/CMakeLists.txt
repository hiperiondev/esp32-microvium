file(
    GLOB_RECURSE
        SOURCES
            source/*.c
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        .
        include
    REQUIRES
        microvium
        uc-hal
)

add_compile_options(-Wno-discarded-qualifiers)