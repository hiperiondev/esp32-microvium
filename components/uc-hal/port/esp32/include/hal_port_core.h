/*
 * @file hal_port_core.h
 * @brief CORE port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_PORT_CORE_H
#define HAL_PORT_CORE_H

#include "hal_core.h"

#define HAL_CORE_PORT_ESP32_INTERNAL_OSC        0
#define HAL_CORE_PORT_ESP32_INTERNAL_OSC        1

/// a variable that keeps count of nested critical sections
extern unsigned int HAL_PORT_InterruptState;

/// implementation of the CORE_EnterCritical macro for nested critical sections
#define CORE_PORT_EnterCritical() // TODO: implement

/// implementation of the CORE_ExitCritical macro for nested critical sections
#define CORE_PORT_ExitCritical() // TODO: implement

/// implementation of the CORE_Deinit macro
#define CORE_PORT_Deinit() // TODO: implement

/// implementation of the HAL_CORE_GetSystemTime macro
#define CORE_PORT_GetSystemTime() // TODO: implement

/// implementation of the CORE_Init macro
void CORE_PORT_Init(void); // TODO: implement

#if defined HAL_CORE_USE_POWER_MANAGEMENT && (HAL_CORE_USE_POWER_MANAGEMENT > 0)
HAL_CORE_POWER_MODES CORE_PORT_SetPowerMode(HAL_CORE_POWER_MODES mode);

HAL_CORE_POWER_MODES CORE_PORT_GetPowerMode(void); // TODO: implement

void CORE_PORT_RefreshPowerMode(void); // TODO: implement
#endif

#if defined HAL_CORE_USE_SYSCLK_MANAGEMENT && (HAL_CORE_USE_SYSCLK_MANAGEMENT > 0)
uint32_t CORE_PORT_SetSystemFreq(uint32_t freq); // TODO: implement
uint32_t CORE_PORT_GetSystemFreq(void); // TODO: implement
uint8_t CORE_PORT_SwitchOsc(uint32_t freq, uint8_t osc_num); // TODO: implement
uint8_t CORE_PORT_GetActiveOscNumber(void); // TODO: implement
uint32_t CORE_PORT_GetActiveOscFreq(void); // TODO: implement

#if defined HAL_CORE_USE_POWER_MANAGEMENT && (HAL_CORE_USE_POWER_MANAGEMENT > 0)
uint8_t CORE_PORT_IsOscAvailable(HAL_CORE_POWER_MODES mode, uint8_t osc_num); // TODO: implement
#endif
#endif

#endif /* HAL_PORT_CORE_H */
