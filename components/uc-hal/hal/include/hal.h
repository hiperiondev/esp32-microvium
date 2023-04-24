/*
 * @file hal.h
 * @brief main header file
 * This is the ONLY file you need to include in application.
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_H
#define HAL_H

#include "hal_config.h"
#include "hal_core.h"
#include "hal_heap.h"

#if (HAL_ENABLE_DIAG)
#include "hal_diag.h"
#endif
#if (HAL_ENABLE_GPIO)
#include "hal_gpio.h"
#endif
#if (HAL_ENABLE_IO)
#include "hal_io.h"
#endif
#if (HAL_ENABLE_TIM)
#include "hal_tim.h"
#endif
#if (HAL_ENABLE_OS)
#include "hal_os.h"
#endif
#if (HAL_ENABLE_BP)
#include "hal_bp.h"
#endif
#if (HAL_ENABLE_NV)
#include "hal_nv.h"
#endif
#if (HAL_ENABLE_WIFI)
#include "hal_wifi.h"
#endif

#endif /* HAL_H */
