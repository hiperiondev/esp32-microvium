/*
 * @file hal_port_usart.h
 * @brief USART port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_PORT_USART_H
#define HAL_PORT_USART_H

#include "hal.h"
#include "hal_io.h"
#include "hal_ioperiph.h"

#if (HALPORT_USE_USART1)
/// IOPeripheral object for USART1
extern IOPeripheralDesc s // TODO: implement
#endif

#if (HALPORT_USE_USART2)
/// IOPeripheral object for USART2
extern IOPeripheralDesc  // TODO: implement
#endif

#if (HALPORT_USE_USART3)
/// IOPeripheral object for USART3
extern IOPeripheralDesc  // TODO: implement
#endif

// -----------------------------------------------------------------------------
//  Section controlling definition dependencies
// -----------------------------------------------------------------------------
//TODO: definition dependencies for hal_port_usart

#endif /* HAL_PORT_USART_H */
