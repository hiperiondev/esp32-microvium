file(
    GLOB_RECURSE
        SOURCES
            source/*.c
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        include
    REQUIRES
        uc-hal
)