/*
 * @file hal_tim.h
 * @brief Textual IODevice
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_TXTDEVICE_H
#define HAL_TXTDEVICE_H

#include <stdint.h>
#include <strings.h>

#include "hal.h"

void TXTDEV_WriteString(IODevice device, const char *str);
void TXTDEV_WriteINT(IODevice device, int32_t value, uint8_t base);
void TXTDEV_WriteUINT(IODevice device, uint32_t value, uint8_t base);
void TXTDEV_WriteNL(IODevice device);
void TXTDEV_ReadString(IODevice input_device, IODevice mirror_device, char *str, size_t max_len);

#endif /* HAL_TXTDEVICE_H */
