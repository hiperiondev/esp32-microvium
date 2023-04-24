/*
 * @file microvium_hal.c
 * @brief microvium HAL module
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include "microvium_hal.h"
#include "microvium_hal_configure.h"

mvm_TeError microvium_hal_resolveImport(mvm_HostFunctionID hostFunctionID, void *context, mvm_TfHostFunction *out_hostFunction) {
    switch (hostFunctionID) {
#ifdef MICROVIUM_HAL_WIFI
        case MICROVIUM_HAL_ID_WIFI_CONNECT_STA:
            *out_hostFunction = &microvium_wifi_connect_sta;
            break;
        case MICROVIUM_HAL_ID_WIFI_IS_CONNECTED:
            *out_hostFunction = &microvium_wifi_IsConnected;
            break;
#endif
        default:
            return MVM_E_FUNCTION_NOT_FOUND;
    }

    return MVM_E_SUCCESS;
}
