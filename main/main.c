/*
 * @file main.c
 *
 * @brief main program
 * @details
 * This is based on other projects:
 *   HALFRED ver. 0.2.0 [bsd license: see LICENSE-HALFRED.txt](http://www.wsn.agh.edu.pl/download/public/halfred/0.2.0/)
 *   Others (see individual files)
 *
 *   please contact their authors for more information.
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 */

#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdbool.h>

#include <esp_err.h>
#include <esp_log.h>

#include "nvs.h"
#include "nvs_flash.h"
#include "esp_partition.h"
#include "hal_fs.h"
#include "hal_wifi.h"
#include "esp32_ftp_server.h"
#include "microvium.h"

#define MICROVIUM_HAL_WIFI
#include "microvium_hal.h"

static char TAG[] = "main";
TaskHandle_t microviumtsk_handle;

#define WIFI_SSID "test"
#define WIFI_PASS "test1234"

#define EP(x) [x] = #x  // enum print
const char *microvium_error[] = {
        EP(MVM_E_SUCCESS),                                     //
        EP(MVM_E_UNEXPECTED),                                  //
        EP(MVM_E_MALLOC_FAIL),                                 //
        EP(MVM_E_ALLOCATION_TOO_LARGE),                        //
        EP(MVM_E_INVALID_ADDRESS),                             //
        EP(MVM_E_COPY_ACROSS_BUCKET_BOUNDARY),                 //
        EP(MVM_E_FUNCTION_NOT_FOUND),                          //
        EP(MVM_E_INVALID_HANDLE),                              //
        EP(MVM_E_STACK_OVERFLOW),                              //
        EP(MVM_E_UNRESOLVED_IMPORT),                           //
        EP(MVM_E_ATTEMPT_TO_WRITE_TO_ROM),                     //
        EP(MVM_E_INVALID_ARGUMENTS),                           //
        EP(MVM_E_TYPE_ERROR),                                  //
        EP(MVM_E_TARGET_NOT_CALLABLE),                         //
        EP(MVM_E_HOST_ERROR),                                  //
        EP(MVM_E_NOT_IMPLEMENTED),                             //
        EP(MVM_E_HOST_RETURNED_INVALID_VALUE),                 //
        EP(MVM_E_ASSERTION_FAILED),                            //
        EP(MVM_E_INVALID_BYTECODE),                            //
        EP(MVM_E_UNRESOLVED_EXPORT),                           //
        EP(MVM_E_RANGE_ERROR),                                 //
        EP(MVM_E_DETACHED_EPHEMERAL),                          //
        EP(MVM_E_TARGET_IS_NOT_A_VM_FUNCTION),                 //
        EP(MVM_E_FLOAT64),                                     //
        EP(MVM_E_NAN),                                         //
        EP(MVM_E_NEG_ZERO),                                    //
        EP(MVM_E_OPERATION_REQUIRES_FLOAT_SUPPORT),            //
        EP(MVM_E_BYTECODE_CRC_FAIL),                           //
        EP(MVM_E_BYTECODE_REQUIRES_FLOAT_SUPPORT),             //
        EP(MVM_E_PROTO_IS_READONLY),                           //
        EP(MVM_E_SNAPSHOT_TOO_LARGE),                          //
        EP(MVM_E_MALLOC_MUST_RETURN_POINTER_TO_EVEN_BOUNDARY), //
        EP(MVM_E_ARRAY_TOO_LONG),                              //
        EP(MVM_E_OUT_OF_MEMORY),                               //
        EP(MVM_E_TOO_MANY_ARGUMENTS),                          //
        EP(MVM_E_REQUIRES_LATER_ENGINE),                       //
        EP(MVM_E_PORT_FILE_VERSION_MISMATCH),                  //
        EP(MVM_E_PORT_FILE_MACRO_TEST_FAILURE),                //
        EP(MVM_E_EXPECTED_POINTER_SIZE_TO_BE_16_BIT),          //
        EP(MVM_E_EXPECTED_POINTER_SIZE_NOT_TO_BE_16_BIT),      //
        EP(MVM_E_TYPE_ERROR_TARGET_IS_NOT_CALLABLE),           //
        EP(MVM_E_TDZ_ERROR),                                   //
        EP(MVM_E_MALLOC_NOT_WITHIN_RAM_PAGE),                  //
        EP(MVM_E_INVALID_ARRAY_INDEX),                         //
        EP(MVM_E_UNCAUGHT_EXCEPTION),                          //
        EP(MVM_E_FATAL_ERROR_MUST_KILL_VM),                    //
        EP(MVM_E_OBJECT_KEYS_ON_NON_OBJECT),                   //
        EP(MVM_E_INVALID_UINT8_ARRAY_LENGTH),                  //
        EP(MVM_E_CAN_ONLY_ASSIGN_BYTES_TO_UINT8_ARRAY),        //
        EP(MVM_E_WRONG_BYTECODE_VERSION),                      //
        EP(MVM_E_USING_NEW_ON_NON_CLASS),                      //
        EP(MVM_E_INSTRUCTION_COUNT_REACHED),                   //
};

const char *wifi_cypher[] = {
        "NONE",        //
        "WEP40",       //
        "WEP104",      //
        "TKIP",        //
        "CCMP",        //
        "TKIP_CCMP",   //
        "AES_CMAC128", //
        "SMS4",        //
        "GCMP",        //
        "GCMP256",     //
        "AES_GMAC128", //
        "AES_GMAC256", //
        "UNKNOWN",     //
};

// A function in the host (this file) for the VM to call
#define IMPORT_PRINT 1

// A function exported by VM to for the host to call
const mvm_VMExportID SAY_HELLO = 1234;

mvm_TeError print(mvm_VM *vm, mvm_HostFunctionID funcID, mvm_Value *result, mvm_Value *args, uint8_t argCount) {
    assert(argCount == 1);
    printf("%s\n", (const char*) mvm_toStringUtf8(vm, args[0], NULL));
    return MVM_E_SUCCESS;
}

/*
 * This function is called by `mvm_restore` to search for host functions
 * imported by the VM based on their ID. Given an ID, it needs to pass back
 * a pointer to the corresponding C function to be used by the VM.
 */
mvm_TeError resolveImport(mvm_HostFunctionID funcID, void *context, mvm_TfHostFunction *out) {
    if (funcID == IMPORT_PRINT) {
        *out = print;
        return MVM_E_SUCCESS;
    }

    return microvium_hal_resolveImport(funcID, context, out);
}

void microvium_task(void *pvParameter) {
    mvm_TeError err;
    mvm_VM *vm;
    uint8_t *snapshot = NULL;
    mvm_Value sayHello;
    mvm_Value result;
    FILE *snapshotFile;
    long snapshotSize;

    // Read the bytecode from file
    ESP_LOGI(TAG, "open file: script.mvm-bc");
    snapshotFile = fs_open("script.mvm-bc", "rb");
    if (snapshotFile == NULL) {
        ESP_LOGI(TAG, "FILE NOT FOUND");
        goto endofall;
    }

    long int prev = ftell(snapshotFile);
    fseek(snapshotFile, 0L, SEEK_END);
    snapshotSize = ftell(snapshotFile);
    fseek(snapshotFile, prev, SEEK_SET);
    ESP_LOGI(TAG, "file length: %ld", snapshotSize);

    snapshot = (uint8_t*) malloc(snapshotSize);
    fread(snapshot, 1, snapshotSize, snapshotFile);
    fclose(snapshotFile);

    // Restore the VM from the snapshot
    err = mvm_restore(&vm, snapshot, snapshotSize, NULL, resolveImport);
    if (err != MVM_E_SUCCESS) {
        ESP_LOGI(TAG, "mvm_restore error: %d [%s]", err, microvium_error[err]);
        goto endofall;
    }

    // Find the "sayHello" function exported by the VM
    err = mvm_resolveExports(vm, &SAY_HELLO, &sayHello, 1);
    if (err != MVM_E_SUCCESS) {
        ESP_LOGI(TAG, "mvm_resolveExports error: %d [%s]", err, microvium_error[err]);
        goto endofall;
    }

    // Call "sayHello"
    err = mvm_call(vm, sayHello, &result, NULL, 0);
    if (err != MVM_E_SUCCESS) {
        ESP_LOGI(TAG, "mvm_call error: %d [%s]", err, microvium_error[err]);
        goto endofall;
    }

    // Clean up
    ESP_LOGI(TAG, "mvm_runGC");
    mvm_runGC(vm, 1);

    ESP_LOGI(TAG, "END");
endofall:
    while (1)
        vTaskDelay(2000 / portTICK_PERIOD_MS);
}

void app_main() {
    nvs_flash_init();
    fs_init();

    printf("Connect WIFI\n");
    wifi_connect_sta(WIFI_SSID, WIFI_PASS);

    // Create FTP server task
    xTaskCreate(
            ftp_task,
            "FTP",
            1024 * 6,
            NULL,
            2,
            NULL
            );

    xTaskCreatePinnedToCore(
            microvium_task,
            "microvium_task",
            15000,
            NULL,
            10,
            &microviumtsk_handle,
            0
    );
}
