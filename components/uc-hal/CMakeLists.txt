file(
    GLOB_RECURSE
        SOURCES
            port/os/FreeRTOS/source*.c
            port/esp32/source/*.c
)

idf_component_register(
    SRCS ${SOURCES}
    INCLUDE_DIRS
        hal/include
        port/os/FreeRTOS/include
        port/esp32
        port/esp32/include
    REQUIRES
        esp_wifi
        littlefs
)