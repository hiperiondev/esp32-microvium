/*
 * @file hal_nv_internal.h
 * @brief Declarations of the types used internally by the NV memory API
 * Putting this declarations in a separate file allows to perform deep unit testing.
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef NVMEM_INTERNAL_H_
#define NVMEM_INTERNAL_H_

#if !defined(HAL_NV_C_) || (HAL_NV_C_ == 0)
#error "The hal_nv_internal.h can be only included from hal_nv.c file."
#endif

#define HAL_NV_CRITICAL_SECTION_DECLARE
#define HAL_NV_CRITICAL_SECTION_BEGIN()     CORE_EnterCritical()
#define HAL_NV_CRITICAL_SECTION_END()       CORE_ExitCritical()

#define HAL_NV_MAX_DELAY 1000UL

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)

/* Binary sempahores */
#define HAL_NV_SEMAPHORE_DECLARE(name)      OSSem name
/* semaphore should be created as "taken" (resource not available) */
#define HAL_NV_SEMAPHORE_CREATE(name)       do { name = OSSEM_Create(); (void) OSSEM_Take(name, 0); } while(0)
#define HAL_NV_SEMAPHORE_DELETE(name)       do { } while(0)
#define HAL_NV_SEMAPHORE_GIVE(name)         OSSEM_Give(name)
#define HAL_NV_SEMAPHORE_TAKE(name)         OSSEM_Take(name, HAL_NV_MAX_DELAY)


#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
/*
 * Counting sempahores (up to number of entries in requests queue)
 * initially semaphore should have counter set to 0 (no entries available)
 */
/*#define HAL_NV_WAIT_SEM_PTR_DECLARE(name) xSemaphoreHandle name*/
#define HAL_NV_WAIT_SEM_CREATE(name, count) name = OSCNTSEM_Create(0, (count))
#define HAL_NV_WAIT_SEM_DELETE(name)        do { } while(0)
#define HAL_NV_WAIT_SEM_POST(name)          OSCNTSEM_Give(name)
#define HAL_NV_WAIT_SEM_WAIT(name)          OSCNTSEM_Take(name, HAL_NV_MAX_DELAY)
#endif /* defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0) */

#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0) */

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)

typedef struct NV_Semaphore_Tag {
    HAL_NV_SEMAPHORE_DECLARE (sem);
    struct NV_SemaphorePool_Tag *pool;
} NV_Semaphore_T;

typedef NV_Semaphore_T *NV_Semaphore;

typedef struct NV_SemaphorePool_Tag {
    NV_Semaphore semaphores;
    uint32_t no_sems;
} NV_SemaphorePool_T;

#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0) */

typedef union NV_OperationData_Tag {
    BP_PartialBuffer op_buf;
    void *mem_ptr;
} NV_OperationData_T;

typedef enum {
    NV_NOP,
    NV_READ,
    NV_SYNC_WRITE,
    NV_ASYNC_WRITE,
    NV_ERASE,
    NV_FLUSH
} NV_OpType;

typedef struct NV_Request_Tag {
    NV_Memory dev;
    NV_OpType op_type;
    NV_Addressable nv_addr;
    NV_OperationData_T data;
    NV_Addressable length;
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
    NV_Semaphore notification;
    #endif
    volatile NV_OpResult *result;
} NV_Request_T;

typedef NV_Request_T *NV_Request;

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
typedef struct NV_RequestQueue_Tag {
    NV_Request requests;
    uint32_t max_no_requests;
    volatile uint32_t pending_requests;
    volatile uint32_t head;
    volatile uint32_t tail;
} NV_RequestQueue_T;
#endif

#endif /* NVMEM_INTERNAL_H_ */
