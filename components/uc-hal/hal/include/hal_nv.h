/*
 * @file hal_nv.h
 * @brief HAL NV memory module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_NV_H_
#define HAL_NV_H_

#include <stdbool.h>

#include "hal_config.h"
#include "hal_os.h"
#include "hal_bp.h"

/** \defgroup hal_nv NV memory module
 *
 * <b>Introduction.</b>
 *
 * The NV module provides API to access a non volatile memory in a uniform manner.
 * This allows to store and retrieve data without knowledge about internals of a physical storage.
 * From the developer perspective there are present some memory device (or devices) with
 * simple write/read access to it.
 * In the multitasking environment (e.g. RTOS) it is also possible to perform
 * the asynchronous write operations (with notification about result).
 * All the read/write operations are always serialized. It means that they will be handled in
 * order of arrival and no operation can be interrupted by another one.
 *
 * <b>Memory device.</b>
 *
 * The memory device is represented by the NV_MemDevice type. It is a handle to the structure
 * holding information required to access (physical) devices which require serialized access
 * e.g. attached to the same communication bus, in general - sharing the same (physical) resource).
 * The (de)initialization and processing NV API is using handle (the NV_MemDevice type) to memory devices.
 *
 * <b>NV Memory.</b>
 *
 * The NV memory is represented by the NV_Memory type. It is a handle to the structure
 * holding information required to access particular physical device (e.g. chip).
 * The NV API for writing and reading is using handle (the NV_Memory type) to NV memory.
 *
 * <b>NV setup.</b>
 *
 * To setup the NV module the application shall initialize appropriate data (NV memories and memory devices)
 * and then call the NV_MemDeviceDeInit function.
 * In case when there is enabled asynchronous write operation then there should be also created
 * a semaphore pool.
 * The user should take into account that initialization procedure uses a real dynamic allocation
 * functions and thus it should be avoided when real time tasks are already started.
 * The recommended way of NV initialization is to do it before RTOS scheduler is executed.
 * Below there is an example how to setup NV module for accessing a AT45DB chip attached to the SPI1 IO.
 * \code
 * static uint8_t at45db_buf[NV_AT45DB_SMALL_PAGE_SIZE];
 * NV_AT45DB_MEM_DECLARE(nvmem_at45db, IO_SPI1);
 * static NV_Memory devs[1] = { &nvmem_at45db };
 * static NV_MemDevice_T nvdev_at45db = NV_INIT_MEMDEVICE(devs, 1);
 * nv_buf_pool = BP_Create(9, NV_AT45DB_SMALL_PAGE_SIZE);
 * NV_MemDeviceInit(&nvdev_at45db, 4, 3, at45db_buf, nv_buf_pool);
 * \endcode
 *
 * <b>Using the NV module</b>
 *
 * \code
 * uint8_t rw_buf[2*NV_AT45DB_SMALL_PAGE_SIZE];
 * volatile NV_OpResult async_result;
 * NV_OpResult result;
 * result = NV_WriteSync(&nvmem_at45db, address, sizeof(rw_buf), rw_buf);
 * result = NV_ReadSync(&nvmem_at45db, address, sizeof(rw_buf), rw_buf);
 * result = NV_WriteAsync(&nvmem_at45db, address, sizeof(rw_buf), rw_buf, &async_result);
 * \endcode
 *
 * - <b>Asynchronous write support</b>
 *
 * If we want to use asynchronous write operation then there is required to setup separate OS task
 * to perform operations in background.
 * Keep in mind that priority used for NV worker task is strictly connected to priorities of tasks
 * using NV module and amount of available resources (semaphores and buffers). It is because
 * physical non volatile memory operations are very time consuming (comparing to typical
 * microcontroller operations) and resources are blocked for relatively long periods.
 *
 * \code
 * OSTASK_Create(nv_worker_task, NV_WORKER_TASK_PRIORITY, 2 * configMINIMAL_STACK_SIZE, &nvdev_at45db);
 *
 * static void nv_worker_task(void *pvParameters)
 * {
 *  for ( ;; ) {
 *      NV_ProcessRequests((NV_MemDevice) pvParameters);
 *  }
 * }
 * \endcode
 *
 * <b>Module configuration.</b>
 *
 * To enable the NV module, HAL_ENABLE_NV definition must be set to 1, in hal_config.h.
 *
 * To enable asynchronous operations the HAL_NV_USE_WORKER_TASK must be set to 1,
 * in hal_config.h. In such case there also have to be enabled a buffer pool
 * functionality (HAL_ENABLE_BP).
 * Additionally to enable non polling operation for worker task with semaphore usage
 * the HAL_NV_USE_SEM_TO_PROCESS_IDLE must be set to 1, in hal_config.h (otherwise
 * the function NV_ProcessRequests should be called periodically).
 */

#if !defined(HAL_ENABLE_OS) || (HAL_ENABLE_OS == 0)
#error "OS support has to be enabled to be able to use NVMEM!"
#endif

#if !defined(HAL_ENABLE_BP) || (HAL_ENABLE_BP == 0)
#error "Buffer pool support has to be enabled to be able to use NVMEM!"
#endif

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
/*
 * Counting semaphores (up to number of entries in requests queue)
 * initially semaphore should have counter set to 0 (no entries available)
 */
#define HAL_NV_WAIT_SEM_PTR_DECLARE(name)   OSCntSem name
#endif /* defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0) */
#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0) */

#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
#if !defined(HAL_NV_USE_WORKER_TASK) || (HAL_NV_USE_WORKER_TASK == 0)
#warning "The HAL_NV_USE_SEM_TO_PROCESS_IDLE macro definition has no effect!"
#undef HAL_NV_USE_SEM_TO_PROCESS_IDLE
#endif
#endif

/**
 *  Possible values returned by the API functions from this module.
 */
typedef enum {
    NVOP_OK,
    NVOP_IN_PROGRESS,
    NVOP_BAD_REQUEST,
    NVOP_NO_SEM_AVAIL,
    NVOP_NO_BUF_AVAIL,
    NVOP_TOO_MANY_REQ,
    NVOP_DEVOP_RD_ERR,
    NVOP_DEVOP_WR_ERR,
    NVOP_DEVOP_ER_ERR,
    NVOP_LOCKED
} NV_OpResult;

/**
 *  Type for values used as addresses or offsets in the NV memory devices.
 */
typedef uint32_t NV_Addressable;

/**
 *  Type for values used as device IDs.
 */
typedef uint32_t NV_DevId;

/**
 *  Type for handle used to access the NVMemory objects.
 */
struct NV_Memory_Tag;
typedef struct NV_Memory_Tag *NV_Memory;

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)

/**
 *  Type for handle used to access the SemaphorePool objects.
 */
struct NV_SemaphorePool_Tag;
typedef struct NV_SemaphorePool_Tag *NV_SemaphorePool;

/**
 *  Type for handle used to access the RequestQueue objects.
 */
struct NV_RequestQueue_Tag;
typedef struct NV_RequestQueue_Tag *NV_RequestQueue;

#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0) */

/**
 *  Type for keeping the properties of the logical NV memory device.
 *  The properties are:
 *  start_addr: it is a base address used to access data on device
 *  end_addr: the end data address available in the device (the addressing between start_addr and end_addr is continous)
 *  write_len_unit: length of the physical write operation for described device
 */
typedef struct NV_AddressMap_Tag {
    NV_Addressable start_addr;
    NV_Addressable end_addr;
    NV_Addressable write_len_unit;
} NV_AddressMap_T;

/**
 *  Type for handle used to access the NVMemory objects.
 */
typedef NV_AddressMap_T *NV_AddressMap;

/**
 *  Type for properties of the MemPeripheral object.
 */
typedef struct NV_MemPeripheral_Tag {
    void (*init)(NV_Memory dev);
    void (*deinit)(NV_Memory dev);
    NV_OpResult (*read)(NV_Memory dev, NV_Addressable addr, NV_Addressable size, void *dst);
    NV_OpResult (*write)(NV_Memory dev, NV_Addressable addr, const void *src);
    NV_OpResult (*erase)(NV_Memory dev);
} NV_MemPeripheral_T;

/**
 *  Type for handle used to access the NVMemory objects.
 */
typedef NV_MemPeripheral_T *NV_MemPeripheral;

/**
 *  Type for properties of the NV_MemDevice object.
 */
typedef struct NV_MemDevice_Tag {
    NV_Memory *devices;
    uint32_t no_devices;
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
    NV_RequestQueue dev_requests;
    NV_SemaphorePool dev_semaphores;
    BP_BufferPool buf_pool;
#endif
    uint8_t *page_buffer;
    volatile bool op_in_progress;
    bool was_init;
    volatile bool lock;
#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
    HAL_NV_WAIT_SEM_PTR_DECLARE (req_queue_sem);
#endif
} NV_MemDevice_T;

/**
 *  Type for handle used to access the NVMemory objects.
 */
typedef NV_MemDevice_T *NV_MemDevice;

/**
 *  Type for properties of the NvMemory object.
 */
typedef struct NV_Memory_Tag {
    NV_MemDevice parent_dev;
    NV_AddressMap mem_map;
    void *phy_data;
    NV_MemPeripheral ops;
} NV_Memory_T;

/**
 *  Initializer ("constructor") for the the NvMemory object.
 *  It should be used as initializer in declaration of the NV_Memory_T object.
 *
 *  @param mem_map      an NV_AddressMap_T object with definition of memory layout (for some devices
 *                      its content is initialized automatically)
 *  @param phy_data     a pointer to physical data associated with NV_Memory object (e.g. NV_AT45DB_PhyData)
 *  @param dev_ops      an NV_MemPeripheral object with definitions of operations for given NV_Memory type of device
 */
#define NV_INIT_MEMORY(mem_map, phy_data, dev_ops)  { NULL, &(mem_map), (void*)(phy_data), &(dev_ops) }

/**
 *  Initializer ("constructor") for the the NV_MemDevice object.
 *  It should be used as initializer in declaration of the NV_MemDevice_T object.
 *
 *  @param devs         an array of pointers to NV_Memory objects
 *  @param no_devs      length of the \c devs array
 */
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
#define NV_INIT_MEMDEVICE(devs, no_devs)                { (devs), (no_devs), NULL, NULL, NULL, NULL, false, false, false, NULL }
#else
#define NV_INIT_MEMDEVICE(devs, no_devs)                { (devs), (no_devs), NULL, NULL, NULL, NULL, false, false, false }
#endif
#else
#define NV_INIT_MEMDEVICE(devs, no_devs)                { (devs), (no_devs), NULL, false, false, false }
#endif

/**
 *  Initializes the NV_MemDevice object.
 *  This functions initializes memory device and associated NvMemory objects.
 *  In case when asynchronous operations are enabled then it also allocates request queue and semaphore pool objects.
 *  This function shall to be called before using any other API functions from this module.
 *
 *  @param mdev             NV memory device handle
 *  @param buffer           buffer used for physical operations, its size have to be large enough to keep the whole write unit for
 *                          a given physical device (i.e. page size)
 *  @param req_queue_len    length of the requests queue (only when asynchronous operations are enabled) - the request queue
 *                          should be large enough to keep requests from all tasks using NV module, in case of asynchronous
 *                          requests this parameter should be carefully tuned
 *  @param sem_pool_len     size of the semaphore pool (only when asynchronous operations are enabled) - the semaphores from
 *                          pool are used for handling synchronous requests, this pool should be large enough to allow
 *                          as many as designed of the synchronous operations
 *  @param bpool            buffer pool (only when asynchronous operations are enabled) - these buffers are used to keep
 *                          asynchronous requests data (until is is serviced)
 */
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
void NV_MemDeviceInit(NV_MemDevice mdev, uint8_t* buffer, uint32_t req_queue_len, uint32_t sem_pool_len, BP_BufferPool bpool);
#else
void NV_MemDeviceInit(NV_MemDevice mdev, uint8_t *buffer);
#endif

/**
 *  Deinitializes the NV_MemDevice object.
 *  After this function is called no further API calls will be allowed for given NV_MemDevice.
 *
 *  @param mdev     a memory device handle
 */
void NV_MemDeviceDeInit(NV_MemDevice mdev);

/**
 *  Temporary locks the NV_MemDevice object.
 *  All API functions called for locked object will immediately return with NVOP_LOCKED error code.
 *  To unlock device the NV_MemDeviceUnlock should be called.
 *
 *  @param mdev     a memory device handle
 *  @param flush    a flag to indicate if all operations should be finished before the device
 *                  is locked (only when asynchronous operations are enabled)
 *
 *  @return         NVOP_OK if there was no errors or NVOP_BAD_REQUEST if parameters are worng
 *                  or NVOP_IN_PROGRESS if there is (physical) operation in progress (only if
 *                  asynchronous operations are enabled )
 */
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
NV_OpResult NV_MemDeviceLock(NV_MemDevice mdev, bool flush);
#else
NV_OpResult NV_MemDeviceLock(NV_MemDevice mdev);
#endif

/**
 *  Unlocks previously locked the NV_MemDevice object.
 *  If given object was not locked by the NV_MemDeviceLock function then no action is taken.
 *
 *  @param mdev     a memory device handle
 */
void NV_MemDeviceUnlock(NV_MemDevice mdev);

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
/**
 *  This function is a main "processor" for requests for given NV_MemDevice.
 *  If the configuration parameter HAL_NV_USE_SEM_TO_PROCESS_IDLE is defined and has value other than 0
 *  then in case when there is no requests for a given NV_MemDevice then this function will wait using semaphore.
 *  In case when HAL_NV_USE_SEM_TO_PROCESS_IDLE is not defined then this function should be called periodically.
 *  This function should be called from the task which itself cannot perform any calls to access NV memory device.
 *
 *  @param mdev     a memory device handle
 */
void NV_ProcessRequests(NV_MemDevice mdev);
#endif

/**
 *  This function synchronously reads data from the NV memory.
 *
 *  @param nv_mem   a NV memory handle
 *  @param addr     address to read from
 *  @param size     the length of read operation
 *  @param dst      the destination location for read data (its length should be at least "size" bytes)
 *
 *  @return         status of the read operation
 */
NV_OpResult NV_ReadSync(NV_Memory nv_mem, NV_Addressable addr, NV_Addressable size, void *dst);

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
/**
 *  This function asynchronously writes data to the NV memory.
 *
 *  @param nv_mem   a NV memory handle
 *  @param addr     address to write to
 *  @param size     the length of write operation
 *  @param src      the source location for written data (its length should be at least "size" bytes)
 *  @param result   if this parameter is not NULL then NV module will asynchronously update pointed value with actual state of
 *                  write operation, initially it has assigned value of NVOP_IN_PROGRESS (if request is successfully
 *                  placed into request queue)
 *
 *  @return         in case when request is successfully placed into request queue then NVOP_OK value is returned
 */
NV_OpResult NV_WriteAsync(NV_Memory nv_mem, NV_Addressable addr, NV_Addressable size, const void *src, volatile NV_OpResult *result);
#endif

/**
 *  This function synchronously writes data to the NV memory.
 *
 *  @param nv_mem   a NV memory handle
 *  @param addr     address to write to
 *  @param size     the length of write operation
 *  @param src      the source location for written data (its length should be at least "size" bytes)
 *
 *  @return         status of the write operation
 */
NV_OpResult NV_WriteSync(NV_Memory nv_mem, NV_Addressable addr, NV_Addressable size, const void *src);

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
/**
 *  This function synchronously wait until all previously requests asynchronous write operations are finished.
 *
 *  @param nv_mem   a NV memory handle
 *
 *  @return         status of the flush operation
 */
NV_OpResult NV_Flush(NV_Memory nv_mem);
#endif

/**
 *  This function synchronously erases the whole device.
 *  The value placed in the NV memory after the operation is dependent on attached physical device.
 *
 *  @param nv_mem   a NV memory handle
 *
 *  @return         status of the erase operation
 */
NV_OpResult NV_Erase(NV_Memory nv_mem);

#endif /* HAL_NV_H_ */

