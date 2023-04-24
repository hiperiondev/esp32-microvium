/*
 * @file hal_core.h
 * @brief CORE module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_CORE_H
#define HAL_CORE_H

#include <stdint.h>

#include "hal_config.h"

/** \defgroup hal_core CORE module
 * 
 * <b>Introduction.</b>
 * 
 * This module implements the basic functionality of a processor core.
 * It provides the main initialization and deinitialization functions,
 * that can be used to implement startup/system setup and safe shutdown.
 * It also provides necessary abstraction functions to implement atomic
 * code blocks (critical sections).
 * 
 * <b>Initialization and deinitialization.</b>
 * 
 * The CORE initialization is used to initialize the port of HAL. The exact
 * functionality behind this may vary with processor architecture. Some
 * examples include configuring power management, clocks, internal PLLs etc.
 * Note, that \ref CORE_Init invokes port's <b>CORE_PORT_Init</b>.
 * 
 * The deinitialization of CORE module also strongly depends on the 
 * system architecture. Typical examples of \ref CORE_Deinit usage would be:
 * shutting down voltage regulators, external reset, dumping volatile memories
 * etc. 
 * 
 * <b>Critical sections.</b>
 * 
 * Very often it is necessary to make a block of code indivisible. Such blocks
 * are called critical sections. The implementation of critical section is
 * processor and system dependent, however the most common way of achieving
 * such behavior is to block (disable) all interrupts at the beginning of
 * the atomic code block and re-enable them at it's end. In such implementation
 * the \ref CORE_EnterCritical would disable interrupts and \ref CORE_ExitCritical
 * would re-enable them. Some care must be taken in order to implement nestable 
 * critical section, however the HAL interface does not specify whether they
 * should or shouldn't be nestable. It's up to the user, or more precisely: 
 * HAL port developer.
 * 
 * <b>System clock management.</b>
 * 
 * <b>Power management.</b>
 * 
 */
/*@{*/

#if defined HAL_CORE_USE_POWER_MANAGEMENT && (HAL_CORE_USE_POWER_MANAGEMENT > 0)
typedef enum {
    /**
     * Default power mode for the port. This usually maps to ACTIVE mode.
     * Check port documentation for details.
     */
    HAL_CORE_POWER_MODE_DEFAULT = 0,
    /**
     * 
     */
    HAL_CORE_POWER_MODE_ACTIVE,
    /**
     * 
     */
    HAL_CORE_POWER_MODE_LOW_POWER,
    /**
     * 
     */
    HAL_CORE_POWER_MODE_SLEEP,
    /**
     *
     */
    HAL_CORE_POWER_MODE_HIBERNATE,
    /**
     * The core can be waken up just by reset or any other event
     * with this same result. After wake-up the program starts from 
     * the beginning.
     */
    HAL_CORE_POWER_MODE_STOP
} HAL_CORE_POWER_MODES;

#endif // HAL_CORE_USE_POWER_MANAGEMENT

/** 
 * Initializes HAL and all the underlying hardware 
 */
#define CORE_Init()                 CORE_PORT_Init()

/** 
 * Deinitializes HAL and all the underlying hardware 
 */
#define CORE_Deinit()               CORE_PORT_Deinit()

/**
 * Starts a critical section 
 */
#define CORE_EnterCritical()        CORE_PORT_EnterCritical()

/** 
 * Ends a critical section
 */
#define CORE_ExitCritical()         CORE_PORT_ExitCritical()

#if (defined HAL_ENABLE_OS) && (HAL_ENABLE_OS)
#define CORE_GetSystemTime()        OS_PORT_GetSystemTime()
#else
/**
 * Returns the current system time (usually the number of system ticks that elapsed since reset.
 * If OS module is enabled, the implementation of this function is the HAL_OS_PORT_GetSystemTime, else
 * the implementation is actually CORE_PORT_GetSystemTime
 */
#define CORE_GetSystemTime()        CORE_PORT_GetSystemTime()
#endif

#if defined HAL_CORE_USE_POWER_MANAGEMENT && (HAL_CORE_USE_POWER_MANAGEMENT > 0)
/** 
 * Sets processor power mode
 * This function should never be called from Interrupt!
 *
 *  @param mode power mode that we want to set
 *  @return set mode or -1 if this operation was not possible
 */
#define CORE_SetPowerMode(mode)         CORE_PORT_SetPowerMode(mode)

/**
 * Gets processor power mode
 */
#define CORE_GetPowerMode()             CORE_PORT_GetPowerMode()

/**
 * Refreshes processor power mode
 */
#define CORE_RefreshPowerMode()         CORE_PORT_RefreshPowerMode()

#endif // HAL_CORE_USE_POWER_MANAGEMENT

#if defined HAL_CORE_USE_SYSCLK_MANAGEMENT && (HAL_CORE_USE_SYSCLK_MANAGEMENT > 0)
/**
 * Sets processor system frequency
 *
 *  @param freq system frequency that we want to set
 *  @return set frequency or -1 if this operation was not possible
 */
#define CORE_SetSystemFreq(freq)            CORE_PORT_SetSystemFreq(freq)

/**
 * Gets processor system frequency
 */
#define CORE_GetSystemFreq()                CORE_PORT_GetSystemFreq()

/**
 * Enables specified oscillator (if several oscillators are supported)
 *
 *  @param freq frequency of the specified oscillator
 *  @param osc_num oscillator number, 0 always sets default oscillator
 * it depends of device port core in which mode which oscillator
 * can be used
 *  @return set oscillator number or -1 if this operation in not possible
 * in current power mode
 */
#define CORE_SwitchOsc(freq, osc_num)       CORE_PORT_SwitchOsc(freq, osc_num)

/**
 * Gets number of the oscillator that is currently used
 *
 *  @return currently used oscillator number
 */
#define CORE_GetActiveOscNumber()           CORE_PORT_GetActiveOscNumber()

/**
 * Gets frequency of the oscillator that is currently used
 *
 *  @return currently used oscillator frequency
 */
#define CORE_GetActiveOscFreq()             CORE_PORT_GetActiveOscFreq()

#if defined HAL_CORE_USE_POWER_MANAGEMENT && (HAL_CORE_USE_POWER_MANAGEMENT > 0)
/**
 * Returns 1 if specified oscillator can be used in specified mode
 * else 0
 *
 *  @return boolean state of availability of the specified oscillator
 *  in specified mode
 */
#define CORE_IsOscAvailable(mode, osc_num)  CORE_PORT_IsOscAvailable(mode, osc_num)

#endif // HAL_CORE_USE_POWER_MANAGEMENT

#endif // HAL_CORE_USE_SYSCLK_MANAGEMENT

/*@}*/

#include "hal_port_core.h"

#endif /* HAL_CORE_H */
