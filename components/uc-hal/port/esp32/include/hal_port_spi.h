/*
 * @file hal_port_spi.h
 * @brief SPI port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_PORT_SPI_H
#define HAL_PORT_SPI_H

#include "hal_ioperiph.h"

#if defined HALPORT_USE_SPI1 && HALPORT_USE_SPI1
/// IOPeripheral object for SPI1
extern IOPeripheralDesc // TODO: implement
#endif

#if defined HALPORT_USE_SPI1 && HALPORT_USE_SPI2
/// IOPeripheral object for SPI2
extern IOPeripheralDesc // TODO: implement
#endif

#endif /* HAL_PORT_SPI_H */
