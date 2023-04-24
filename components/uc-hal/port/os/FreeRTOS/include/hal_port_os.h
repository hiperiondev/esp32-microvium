/*
 * @file hal_port_os.h
 * @brief FreeRTOS port
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_PORT_OS_H
#define HAL_PORT_OS_H

#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"

#define OS_PORT_MS_TO_TICK(ms_time)             ((ms_time * configTICK_RATE_HZ)/1000)

// -----------------------------------------------------------------------------
//  OS CORE PORT
// -----------------------------------------------------------------------------

typedef TickType_t OSTime;

#define OS_PORT_Start()                         vTaskStartScheduler()
#define OS_PORT_Stop()                          vTaskEndScheduler()
#define OS_PORT_Sleep(ticks)                    vTaskDelay(ticks)
#define OS_PORT_GetSystemTime()                 xTaskGetTickCount()
void OS_PORT_SleepUntil(OSTime time);

// -----------------------------------------------------------------------------
//  OS TASK PORT
// -----------------------------------------------------------------------------

typedef TaskHandle_t OSTask;

OSTask OSTASK_PORT_Create(TaskFunction_t task_impl, int priority, size_t stack_size, void *arg);

#define OSTASK_PORT_Destroy(task)               vTaskDelete((task))
#define OSTASK_PORT_Suspend(task)               vTaskSuspend((task))
#define OSTASK_PORT_Resume(task)                vTaskResume((task))
#define OSTASK_PORT_Yield()                     taskYIELD()
#define OSTASK_PORT_SetPriority(task, priority) vTaskPrioritySet((task),(priority))
#define OSTASK_PORT_GetPriority(task)           vTaskPriorityGet((task))
#define OSTASK_PORT_GetCurrentTask()            xTaskGetCurrentTaskHandle()

// -----------------------------------------------------------------------------
//  OS SEM PORT
// -----------------------------------------------------------------------------

typedef SemaphoreHandle_t OSSem;

#define OSSEM_PORT_Create()                     xQueueCreate((unsigned portBASE_TYPE)1, semSEMAPHORE_QUEUE_ITEM_LENGTH)
#define OSSEM_PORT_Destroy(sem)                 do { } while (0)
#define OSSEM_PORT_Give(sem)                    xSemaphoreGive((sem))
#define OSSEM_PORT_Take(sem, timeout)           ((xSemaphoreTake((sem), OS_PORT_MS_TO_TICK(timeout)) == pdTRUE) ? 0 : 1)

// -----------------------------------------------------------------------------
//  OS CNTSEM PORT
// -----------------------------------------------------------------------------

typedef SemaphoreHandle_t OSCntSem;

#define OSCNTSEM_PORT_Create(init, max)         xSemaphoreCreateCounting((max), (init))
#define OSCNTSEM_PORT_Destroy(sem)              do { } while (0)
#define OSCNTSEM_PORT_Give(sem)                 xSemaphoreGive((sem))
#define OSCNTSEM_PORT_Take(sem, timeout)        OSSEM_PORT_Take((sem),(timeout))

// -----------------------------------------------------------------------------
//  OS MUTEX PORT
// -----------------------------------------------------------------------------

typedef SemaphoreHandle_t OSMutex;

#define OSMUTEX_PORT_Create()                   xSemaphoreCreateMutex()
#define OSMUTEX_PORT_Destroy(mutex)             do { } while (0)
#define OSMUTEX_PORT_Give(mutex)                xSemaphoreGive((mutex))
#define OSMUTEX_PORT_Take(mutex, timeout)       OSSEM_PORT_Take((mutex),(timeout))

#endif // HAL_PORT_OS_H
