/*
 * @file hal_io.h
 * @brief HAL IO module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_IO_H
#define HAL_IO_H

/** \defgroup hal_io IO module
 * 
 * <b>Introduction.</b>
 * 
 * The IO module is responsible for handling communication through several common
 * peripherals, such as UART or SPI. It provides a unified high-level application 
 * interface for different devices and is capable of working in several modes, 
 * taking advantage of additional microcontroler's mechanisms, such as interrupts
 * or DMA channels. The IO module interface is flexible enough to handle most of
 * the common data transmission tasks, but may be also used and abused in different 
 * areas of application.
 * 
 * <b>IO Interfaces.</b>
 * 
 * The main IO interface that the application interacts with is the \ref hal_iodevice.
 * This interface encapsulates all the functionality provided by the IO module.
 * The second interface is the \ref hal_ioperipheral that represents the actual
 * peripheral. This interface should not be used by the application. It the interface
 * that the port must compatible with, in order to be used by IO module.
 * 
 * <b>Porting HAL IO.</b>
 * 
 * TBD
 * 
 * <b>Module configuration.</b>
 * 
 * To enable the IO module, HAL_ENABLE_IO definition must be set to 1, 
 * in hal_config.h. Additional definitions may be required to enable specific modes
 * of operation for specific ports, or other options. Refer to the port documentation.
 *  
 */
/*@{*/

#include "hal_iodevice.h"
#include "hal_port_io.h"

/**
 * Declares statically (at compilation time) an instance of IODevice (object), bonded 
 * with a specified peripheral
 * @param name name of the IODevice object
 * @param peripheral IOPeripheral to bond with this IODevice instance
 */
#define HAL_DECLARE_IODEV(name, peripheral)     IODeviceDesc name = {peripheral, NULL, NULL, NULL, NULL}

/*@}*/

#endif /* HAL_IO_H */

