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
#include "esp32_tftp_server.h"
#include "microvium.h"

static char TAG[] = "main";
TaskHandle_t microviumtsk_handle;

#define WIFI_SSID "test"
#define WIFI_PASS "test1234"

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
    return MVM_E_UNRESOLVED_IMPORT;
}

void microvium_task(void *pvParameter) {
    mvm_TeError err;
    mvm_VM *vm;
    uint8_t *snapshot;
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

    fseek(snapshotFile, 0L, SEEK_END);
    snapshotSize = ftell(snapshotFile);
    rewind(snapshotFile);
    snapshot = (uint8_t*) malloc(snapshotSize);
    fread(snapshot, 1, snapshotSize, snapshotFile);
    fclose(snapshotFile);

    // Restore the VM from the snapshot
    err = mvm_restore(&vm, snapshot, snapshotSize - 1, NULL, resolveImport);
    if (err != MVM_E_SUCCESS) {
        ESP_LOGI(TAG, "mvm_restore error: %d (snapshotSize: %ld)", err, snapshotSize);
        goto endofall;
    }

    // Find the "sayHello" function exported by the VM
    err = mvm_resolveExports(vm, &SAY_HELLO, &sayHello, 1);
    if (err != MVM_E_SUCCESS) {
        ESP_LOGI(TAG, "mvm_resolveExports error: %d", err);
        goto endofall;
    }

    // Call "sayHello"
    err = mvm_call(vm, sayHello, &result, NULL, 0);
    if (err != MVM_E_SUCCESS) {
        ESP_LOGI(TAG, "mvm_call error: %d", err);
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
    hal_wifi_ap_record_t *ap_record = NULL;

    nvs_flash_init();
    fs_init();

    uint32_t list = wifi_scan(&ap_record);
    uint32_t ssid_len = 0;
    for (uint32_t n = 0; n < list; n++)
        if (strlen((char*)ap_record[n].ssid) > ssid_len)
            ssid_len = strlen((char*)ap_record[n].ssid);
    char ssid[ssid_len + 1];
    printf("\nScan WIFI\n");
    for (uint32_t n = 0; n < list; n++) {
        strcpy(ssid, (char*)ap_record[n].ssid);
        memset(ssid + strlen(ssid), ' ', ssid_len - strlen((char*)ap_record[n].ssid));
        ssid[ssid_len] = '\0';
        printf("    > %s [RSSI: %02d] (cipher: %s)\n", ssid, ap_record[n].rssi, wifi_cypher[ap_record[n].group_cipher]);
    }
    printf("\n");
    free(ap_record);

    printf("Connect WIFI\n");
    wifi_connect_sta(WIFI_SSID, WIFI_PASS);
    TFTP_task_start();

    xTaskCreatePinnedToCore(
            microvium_task,
            "microvium_task",
            5000,
            NULL,
            10,
            &microviumtsk_handle,
            0
    );
}
