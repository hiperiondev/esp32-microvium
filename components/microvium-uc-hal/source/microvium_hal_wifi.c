/*
 * @file microvium_hal_wifi.c
 * @brief microvium HAL wifi module
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#define HAL_ENABLE_WIFI 1

#include "hal.h"
#include "microvium_hal.h"
#include "microvium_hal_wifi.h"
#include "microvium.h"

mvm_TeError microvium_wifi_connect_sta(mvm_VM *vm, mvm_HostFunctionID hostFunctionID, mvm_Value *result, mvm_Value *args, uint8_t argCount) {
    if (argCount < 2)
        return MVM_E_UNEXPECTED;

    size_t name_len, pass_len;
    const char *name = mvm_toStringUtf8(vm, args[0], &name_len);
    const char *pass = mvm_toStringUtf8(vm, args[1], &pass_len);

    if (name_len < 1)
        return MVM_E_UNEXPECTED;

    wifi_connect_sta(name, pass);

    return MVM_E_SUCCESS;
}
