/*
 * @file hal_port_gpio.h
 * @brief GPIO port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_PORT_GPIO_H
#define HAL_PORT_GPIO_H

#include "stdint.h"

// -----------------------------------------------------------------------------
//  PUBLIC MACROS
// -----------------------------------------------------------------------------

void HAL_GPIO_PORT_Deinit(void); // TODO: implement
#define HAL_GPIO_PORT_GetConfig(port, pin) // TODO: implement
uint8_t HAL_GPIO_PORT_ReadPin(uint32_t *port, uint8_t pin); // TODO: implement
#define HAL_GPIO_PORT_SetPinHigh(port, pin) // TODO: implement
#define HAL_GPIO_PORT_SetPinLow(port, pin) // TODO: implement
#define HAL_GPIO_PORT_TogglePin(port, pin) // TODO: implement
#define HAL_GPIO_PORT_ReadPort(port) // TODO: implement
#define HAL_GPIO_PORT_WritePort(port, data) // TODO: implement
#define HAL_GPIO_PORT_SetPortHigh(port, data) // TODO: implement)
#define HAL_GPIO_PORT_SetPortLow(port, data) // TODO: implement
void HAL_GPIO_PORT_Init(void);
void HAL_GPIO_PORT_ConfigurePin(uint32_t *port, uint8_t pin, uint32_t mode); // TODO: implement
void HAL_GPIO_PORT_ConfigurePort(uint32_t *port, uint32_t port_mask, uint32_t mode); // TODO: implement
void HAL_GPIO_PORT_EnableEvent(uint32_t *port, uint8_t pin, uint8_t source); // TODO: implement
#define HAL_GPIO_PORT_DisableEvent(port, pin) // TODO: implement
void HAL_GPIO_PORT_SetEventHandler(uint32_t *port, uint8_t pin, void (*handler)(void)); // TODO: implement
int HAL_GPIO_PORT_IsEventEnabled(uint32_t *port, uint8_t pin); // TODO: implement

#endif /* HAL_PORT_GPIO_H */
