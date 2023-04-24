/*
 * @file microvium_hal.h
 * @brief microvium HAL module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_MICROVIUM_HAL_H_
#define HAL_MICROVIUM_HAL_H_

#include "microvium.h"

#ifdef MICROVIUM_HAL_WIFI
#define HAL_ENABLE_WIFI
#include "microvium_hal_wifi.h"
#endif

mvm_TeError microvium_hal_resolveImport(mvm_HostFunctionID hostFunctionID, void *context, mvm_TfHostFunction *out_hostFunction);

#endif /* HAL_MICROVIUM_HAL_H_ */
