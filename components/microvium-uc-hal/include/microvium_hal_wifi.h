/*
 * @file microvium_hal_wifi.h
 * @brief microvium HAL wifi module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef MICROVIUM_HAL_WIFI_H_
#define MICROVIUM_HAL_WIFI_H_

enum MICROVIUM_HAL_ID_WIFI {
    MICROVIUM_HAL_ID_WIFI_CONNECT_STA = 65535,
};

mvm_TeError microvium_wifi_connect_sta(mvm_VM* vm, mvm_HostFunctionID hostFunctionID, mvm_Value* result, mvm_Value* args, uint8_t argCount);

#endif /* MICROVIUM_HAL_WIFI_H_ */
