/*
 * @file hal_port_os.c
 * @brief FreeRTOS port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/portmacro.h"

#include "hal_port_os.h"
#include "hal_heap.h"
#include "hal_diag.h"

// -----------------------------------------------------------------------------
//  OS_PORT_SleepUntil
// -----------------------------------------------------------------------------
void OS_PORT_SleepUntil(OSTime time) {
    OSTime ostime;

    ostime = xTaskGetTickCount();
    time = time - ostime;
    vTaskDelayUntil(&ostime, time);
} /* OS_PORT_SleepUntil */

// -----------------------------------------------------------------------------
//  OSTASK_PORT_Create
// -----------------------------------------------------------------------------
OSTask OSTASK_PORT_Create(TaskFunction_t task_impl, int priority, size_t stack_size, void *arg) {
    OSTask task;
#if defined configUSE_APPLICATION_TASK_TAG && configUSE_APPLICATION_TASK_TAG > 0
    static int os_cnt;
#endif

    if (stack_size == 0) {
        stack_size = configMINIMAL_STACK_SIZE;
    }
    priority += tskIDLE_PRIORITY;
    if (pdPASS != xTaskCreate(task_impl, NULL, stack_size, arg, priority, &task)) {
        return NULL;
    }

#if defined configUSE_APPLICATION_TASK_TAG && configUSE_APPLICATION_TASK_TAG > 0
    vTaskSetApplicationTaskTag(task, (void*) os_cnt++);
#endif

    return task;
} /* OSTASK_PORT_Create */

#if defined HAL_HEAP_MODE && HAL_HEAP_MODE == 1

// -----------------------------------------------------------------------------
//  pvPortMalloc
// -----------------------------------------------------------------------------
void* pvPortMalloc(size_t xWantedSize) {
    // just a wrapper on HAL heap manager
    return HEAP_Alloc(xWantedSize);
} /* pvPortMalloc */

// -----------------------------------------------------------------------------
//  vPortFree
// -----------------------------------------------------------------------------
void vPortFree(void *pv) {
    // just a wrapper on HAL heap manager
    HEAP_Free(pv);
} /* vPortFree */

#endif // HAL_HEAP_MODE == 1

#if defined configCHECK_FOR_STACK_OVERFLOW && (configCHECK_FOR_STACK_OVERFLOW != 0)

// -----------------------------------------------------------------------------
//  vApplicationStackOverflowHook
// -----------------------------------------------------------------------------
/*
 void vApplicationStackOverflowHook(TaskHandle_t *pxTask, signed portCHAR *pcTaskName ) {
 DIAG_LogMsg("STACK OVERFLOW IN OS TASK: ");
 DIAG_LogMsg((char*)pcTaskName);
 DIAG_LogNL();

 } */
/* vApplicationStackOverflowHook */

#endif // configCHECK_FOR_STACK_OVERFLOW != 0
