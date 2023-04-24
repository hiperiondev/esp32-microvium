/*
 * @file hal_defs.h
 * @brief additional HAL definitions
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_DEFS
#define HAL_DEFS

/**
 * HAL result type (common for all HAL modules)
 */
typedef enum {
    HALRESULT_OK = 0,
    HALRESULT_ERROR,
    HALRESULT_NO_EFFECT,
    HALRESULT_INVALID_BUFFER_HANDLE,
    HALRESULT_INVALID_USART_PORT,
    HALRESULT_OSMUTEX_CREATION_FAILED,
    HALRESULT_OSSEM_CREATION_FAILED
} HALRESULT;

/**
 * Macro for testing operation result (returns true if operation succeeded)
 */
#define HAL_SUCCESS(x) ((x) == HALRESULT_OK) 

/**
 * Macro for testing operation result (returns true if operation failed)
 */
#define HAL_FAILED(x) (!HAL_SUCCESS(x)) 

#endif // HAL_DEFS
