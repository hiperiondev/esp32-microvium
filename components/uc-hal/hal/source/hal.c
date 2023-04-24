/*
 * @file hal.c
 * @brief HAL
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include "hal_config.h"

#include "hal_heap1.c"
#include "hal_heap2.c"

#if (HAL_ENABLE_IO)
#include "hal_ioperiph.c"
#include "hal_iodevice.c"
#endif

#if ((HAL_ENABLE_IOBUF) || (HAL_ENABLE_IO))
#include "hal_iobuf.c"
#endif

#if (HAL_ENABLE_DIAG)
#include "hal_diag.c"
#endif

#if (HAL_ENABLE_TIM)
#include "hal_tim.c"
#endif

#if (HAL_IO_OS_INTEGRATION)
#include "hal_osnotifier.c"
#endif

#if (HAL_ENABLE_BP)
#include "hal_bp.c"
#endif

#if (HAL_ENABLE_NV)
#include "hal_nv.c"
#endif

