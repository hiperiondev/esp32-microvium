/*
 * @file hal_nv.c
 * @brief Definitions of the NV memory API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "hal_nv.h"

#define HAL_NV_C_   1
#include "hal_nv_internal.h"

#define NV_ASSIGN_RESULT(res, val)  \
            do {                    \
                if (res) {          \
                    *(res) = (val); \
                }                   \
            } while (0)

static void nv_memory_init(NV_Memory dev, NV_MemDevice parent);
static void nv_memory_deinit(NV_Memory dev);

static NV_Addressable nv_get_block_at(NV_AddressMap map, NV_Addressable addr, NV_Addressable *op_addr);
static bool nv_is_block_avail(NV_AddressMap map, NV_Addressable addr, NV_Addressable size);

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
static NV_SemaphorePool nv_sem_pool_create(uint32_t no_sems);
static void nv_sem_pool_delete(NV_SemaphorePool sp);
static NV_Semaphore nv_sem_pool_get_sem(NV_SemaphorePool spool);
static void nv_sem_pool_return_sem(NV_Semaphore sem);

static NV_RequestQueue nv_request_queue_create(uint32_t qlen);
static void nv_request_queue_delete(NV_RequestQueue rq);
static NV_OpResult nv_add_request(NV_RequestQueue q, NV_Memory dev, NV_OpType type, NV_Addressable addr, NV_Addressable size, void* ptr, NV_Semaphore notification, volatile NV_OpResult* result);
static void nv_process_request(NV_Request req);

static NV_Request nv_allocate_request(NV_RequestQueue q);
static NV_Request nv_get_request(NV_RequestQueue q);
#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0) */

static void nv_process_write_request(NV_Request req);

/**************************************************************************************************\
\**************************************************************************************************/
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
void NV_MemDeviceInit(NV_MemDevice mdev, uint8_t* buffer, uint32_t req_queue_len, uint32_t sem_pool_len, BP_BufferPool bpool)
#else
void NV_MemDeviceInit(NV_MemDevice mdev, uint8_t *buffer)
#endif
{
    if (mdev && !mdev->was_init && mdev->no_devices && mdev->devices
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
            && req_queue_len && sem_pool_len && bpool
#endif
            ) {
        uint32_t dev_idx;

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
        mdev->dev_requests = nv_request_queue_create(req_queue_len);
        mdev->dev_semaphores = nv_sem_pool_create(sem_pool_len);
        mdev->buf_pool = bpool;
#endif

        mdev->page_buffer = buffer;
        mdev->op_in_progress = false;
        mdev->lock = false;

        for (dev_idx = 0; dev_idx < mdev->no_devices; dev_idx++) {
            nv_memory_init(mdev->devices[dev_idx], mdev);
        }

#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
        HAL_NV_WAIT_SEM_CREATE(mdev->req_queue_sem, req_queue_len);
#endif

        mdev->was_init = true;
    }

    if (!mdev
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
            || !req_queue_len || !sem_pool_len || !bpool || !mdev->dev_requests || !mdev->dev_semaphores
#endif
            || !mdev->no_devices || !mdev->devices) {
        HAL_NV_CRITICAL_ERROR();
    }
}

/**************************************************************************************************\
\**************************************************************************************************/
void NV_MemDeviceDeInit(NV_MemDevice mdev) {
    if (mdev && mdev->was_init && mdev->no_devices && mdev->devices) {
        uint32_t dev_idx;

        while (NVOP_IN_PROGRESS == NV_MemDeviceLock(mdev, true)) {
            ;
        }

        for (dev_idx = 0; dev_idx < mdev->no_devices; dev_idx++) {
            nv_memory_deinit(mdev->devices[dev_idx]);
        }
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
        nv_sem_pool_delete(mdev->dev_semaphores);
        mdev->dev_semaphores = NULL;
        nv_request_queue_delete(mdev->dev_requests);
        mdev->dev_requests = NULL;
        #if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
            HAL_NV_WAIT_SEM_DELETE(mdev->req_queue_sem);
        #endif
#endif
        mdev->was_init = false;
    }

    if (!mdev) {
        HAL_NV_CRITICAL_ERROR();
    }
}

/**************************************************************************************************\
\**************************************************************************************************/
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
NV_OpResult NV_MemDeviceLock(NV_MemDevice mdev, bool flush)
#else
NV_OpResult NV_MemDeviceLock(NV_MemDevice mdev)
#endif
{
    NV_OpResult retval;

    retval = NVOP_BAD_REQUEST;

    if (mdev) {
        mdev->lock = true;

        retval = NVOP_OK;

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
        if (flush) {
            if (mdev->op_in_progress) {
                retval = NVOP_IN_PROGRESS;
            } else {
                NV_Request req_ptr;
                HAL_NV_CRITICAL_SECTION_DECLARE;

                do {
                    HAL_NV_CRITICAL_SECTION_BEGIN();
                    req_ptr = nv_get_request(mdev->dev_requests);
                    HAL_NV_CRITICAL_SECTION_END();

                    if (req_ptr) {
                        nv_process_request(req_ptr);
                    }
                } while (req_ptr);
            }
        }
#endif
    }

    if (!mdev) {
        HAL_NV_CRITICAL_ERROR();
    }

    return retval;
}

/**************************************************************************************************\
\**************************************************************************************************/
void NV_MemDeviceUnlock(NV_MemDevice mdev) {
    if (mdev && mdev->was_init) {
        mdev->lock = false;
    }

    if (!mdev) {
        HAL_NV_CRITICAL_ERROR();
    }
}

/**************************************************************************************************\
\**************************************************************************************************/
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
void NV_ProcessRequests(NV_MemDevice mdev) {
    if (mdev && mdev->dev_requests) {
        if (!mdev->lock) {
            NV_Request_T req;
            NV_Request req_ptr;
            HAL_NV_CRITICAL_SECTION_DECLARE;

#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
            if (HAL_NV_WAIT_SEM_WAIT(mdev->req_queue_sem)) {
                HAL_NV_CRITICAL_ERROR();
            }
#endif

            HAL_NV_CRITICAL_SECTION_BEGIN();
            req_ptr = nv_get_request(mdev->dev_requests);
            if (req_ptr) {
                memcpy(&req, req_ptr, sizeof(req));
            }
            HAL_NV_CRITICAL_SECTION_END();

            if (req_ptr) {
                mdev->op_in_progress = true;
                nv_process_request(&req);
                mdev->op_in_progress = false;
            }
        }
    }

    if (!mdev || !mdev->dev_requests) {
        HAL_NV_CRITICAL_ERROR();
    }
}
#endif

/**************************************************************************************************\
\**************************************************************************************************/
NV_OpResult NV_ReadSync(NV_Memory nv_mem, NV_Addressable addr, NV_Addressable size, void *dst) {
    NV_OpResult retval;

    retval = NVOP_BAD_REQUEST;

    if (!nv_mem || !nv_mem->parent_dev || !nv_mem->parent_dev->was_init || !size || !dst) {
        HAL_NV_CRITICAL_ERROR();
    }

    if (nv_mem && nv_mem->parent_dev && nv_mem->parent_dev->was_init && size && dst && nv_is_block_avail(nv_mem->mem_map, addr, size)) {
        if (nv_mem->parent_dev->lock) {
            retval = NVOP_LOCKED;
        } else {
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
            NV_Semaphore sem;
            sem = nv_sem_pool_get_sem(nv_mem->parent_dev->dev_semaphores);
            if (sem) {
                retval = nv_add_request(nv_mem->parent_dev->dev_requests, nv_mem, NV_READ, addr, size, dst, sem, &retval);
                if (NVOP_OK == retval) {
                    if (HAL_NV_SEMAPHORE_TAKE(sem->sem)) {
                        HAL_NV_CRITICAL_ERROR();
                    }
                }
                nv_sem_pool_return_sem(sem);
            } else {
                retval = NVOP_NO_SEM_AVAIL;
            }
#else
            retval = nv_mem->ops->read(nv_mem, addr, size, dst);
#endif
        }
    }

    return retval;
}

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
/**************************************************************************************************\
\**************************************************************************************************/
NV_OpResult NV_WriteAsync(NV_Memory nv_mem, NV_Addressable addr, NV_Addressable size, const void *src, volatile NV_OpResult *result) {
    NV_OpResult retval;

    retval = NVOP_BAD_REQUEST;

    if (!nv_mem || !nv_mem->parent_dev || !nv_mem->parent_dev->was_init || !size || !src) {
        HAL_NV_CRITICAL_ERROR();
    }

    if (nv_mem && nv_mem->parent_dev && nv_mem->parent_dev->was_init && size && src && nv_is_block_avail(nv_mem->mem_map, addr, size)) {
        if (nv_mem->parent_dev->lock) {
            retval = NVOP_LOCKED;
            NV_ASSIGN_RESULT(result, retval);
        } else {
            NV_ASSIGN_RESULT(result, NVOP_IN_PROGRESS);
            BP_PartialBuffer buf;
            buf = BP_GetBuffer(nv_mem->parent_dev->buf_pool, size);
            if (buf) {
                (void) BP_CopyToBuf(buf, src, 0, size);
                retval = nv_add_request(nv_mem->parent_dev->dev_requests, nv_mem, NV_ASYNC_WRITE, addr, size, buf, NULL, result);
                if (retval != NVOP_OK) {
                    NV_ASSIGN_RESULT(result, retval);
                }
            } else {
                retval = NVOP_NO_BUF_AVAIL;
                NV_ASSIGN_RESULT(result, retval);
            }
        }
    } else {
        NV_ASSIGN_RESULT(result, retval);
    }

    return retval;
}
#endif

/**************************************************************************************************\
\**************************************************************************************************/
NV_OpResult NV_WriteSync(NV_Memory nv_mem, NV_Addressable addr, NV_Addressable size, const void *src) {
    NV_OpResult retval;

    retval = NVOP_BAD_REQUEST;

    if (!nv_mem || !nv_mem->parent_dev || !nv_mem->parent_dev->was_init || !size || !src) {
        HAL_NV_CRITICAL_ERROR();
    }

    if (nv_mem && nv_mem->parent_dev && nv_mem->parent_dev->was_init && size && src && nv_is_block_avail(nv_mem->mem_map, addr, size)) {
        if (nv_mem->parent_dev->lock) {
            retval = NVOP_LOCKED;
        } else {
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
            NV_Semaphore sem;
            sem = nv_sem_pool_get_sem(nv_mem->parent_dev->dev_semaphores);
            if (sem) {
                BP_PartialBuf_T buf;
                BP_InitStandaloneBuf(&buf, (void*) src, size);
                retval = nv_add_request(nv_mem->parent_dev->dev_requests, nv_mem, NV_SYNC_WRITE, addr, size, &buf, sem, &retval);
                if (NVOP_OK == retval) {
                    if (HAL_NV_SEMAPHORE_TAKE(sem->sem)) {
                        HAL_NV_CRITICAL_ERROR();
                    }
                }
                nv_sem_pool_return_sem(sem);
            } else {
                retval = NVOP_NO_SEM_AVAIL;
            }
#else
            BP_PartialBuf_T buf;
            NV_Request_T req;

            BP_InitStandaloneBuf(&buf, (void*) src, size);
            req.dev = nv_mem;
            req.op_type = NV_SYNC_WRITE;
            req.nv_addr = addr;
            req.data.op_buf = &buf;
            req.length = size;
            req.result = &retval;
            nv_process_write_request(&req);
#endif
        }
    }

    return retval;
}

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
/**************************************************************************************************\
\**************************************************************************************************/
NV_OpResult NV_Flush(NV_Memory nv_mem) {
    NV_OpResult retval;

    retval = NVOP_BAD_REQUEST;

    if (!nv_mem || !nv_mem->parent_dev || !nv_mem->parent_dev->was_init) {
        HAL_NV_CRITICAL_ERROR();
    }

    if (nv_mem && nv_mem->parent_dev && nv_mem->parent_dev->was_init) {
        if (nv_mem->parent_dev->lock) {
            retval = NVOP_LOCKED;
        } else {
            NV_Semaphore sem;
            sem = nv_sem_pool_get_sem(nv_mem->parent_dev->dev_semaphores);
            if (sem) {
                retval = nv_add_request(nv_mem->parent_dev->dev_requests, nv_mem, NV_FLUSH, 0, 0, NULL, sem, &retval);
                if (NVOP_OK == retval) {
                    if (HAL_NV_SEMAPHORE_TAKE(sem->sem)) {
                        HAL_NV_CRITICAL_ERROR();
                    }
                }
                nv_sem_pool_return_sem(sem);
            } else {
                retval = NVOP_NO_SEM_AVAIL;
            }
        }
    }

    return retval;
}
#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0) */

/**************************************************************************************************\
\**************************************************************************************************/
NV_OpResult NV_Erase(NV_Memory nv_mem) {
    NV_OpResult retval;

    retval = NVOP_BAD_REQUEST;

    if (!nv_mem || !nv_mem->parent_dev || !nv_mem->parent_dev->was_init) {
        HAL_NV_CRITICAL_ERROR();
    }

    if (nv_mem && nv_mem->parent_dev && nv_mem->parent_dev->was_init) {
        if (nv_mem->parent_dev->lock) {
            retval = NVOP_LOCKED;
        } else {
#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
            NV_Semaphore sem;
            sem = nv_sem_pool_get_sem(nv_mem->parent_dev->dev_semaphores);
            if (sem) {
                retval = nv_add_request(nv_mem->parent_dev->dev_requests, nv_mem, NV_ERASE, 0, 0, NULL, sem, &retval);
                if (NVOP_OK == retval) {
                    if (HAL_NV_SEMAPHORE_TAKE(sem->sem)) {
                        HAL_NV_CRITICAL_ERROR();
                    }
                }
                nv_sem_pool_return_sem(sem);
            } else {
                retval = NVOP_NO_SEM_AVAIL;
            }
#else
            retval = nv_mem->ops->erase(nv_mem);
#endif
        }
    }

    return retval;
}

/**************************************************************************************************\
\**************************************************************************************************/
static NV_Addressable nv_get_block_at(NV_AddressMap map, NV_Addressable addr, NV_Addressable *op_addr) {
    NV_Addressable retval;

    retval = 0;

    if (map && (addr >= map->start_addr) && (addr <= map->end_addr)) {
        if (op_addr) {
            *op_addr = map->write_len_unit * (addr / map->write_len_unit);
        }
        retval = map->write_len_unit;
    }

    if (!map) {
        HAL_NV_CRITICAL_ERROR();
    }

    return retval;
}

/**************************************************************************************************\
\**************************************************************************************************/
static bool nv_is_block_avail(NV_AddressMap map, NV_Addressable addr, NV_Addressable size) {
    return map && (!size || ((addr >= map->start_addr) && ((addr + (size - 1)) <= map->end_addr)));
}

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)

/**************************************************************************************************\
\**************************************************************************************************/
static NV_SemaphorePool nv_sem_pool_create(uint32_t no_sems) {
    NV_SemaphorePool retval;

    retval = NULL;

    if (no_sems) {
        retval = malloc(sizeof(NV_SemaphorePool_T));

        if (retval) {
            retval->semaphores = malloc(no_sems * sizeof(NV_Semaphore_T));
            if (retval->semaphores) {
                uint32_t sem_idx;
                retval->no_sems = no_sems;
                for (sem_idx = 0; sem_idx < no_sems; sem_idx++) {
                    HAL_NV_SEMAPHORE_CREATE(retval->semaphores[sem_idx].sem);
                    retval->semaphores[sem_idx].pool = NULL;
                }
            } else {
                free(retval);
                retval = NULL;
            }
        }
    }

    return retval;
}

/**************************************************************************************************\
\**************************************************************************************************/
static void nv_sem_pool_delete(NV_SemaphorePool sp) {
    if (sp) {
        uint32_t sem_idx;
        for (sem_idx = 0; sem_idx < sp->no_sems; sem_idx++) {
            HAL_NV_SEMAPHORE_DELETE(sp->semaphores[sem_idx].sem);
        }
        free(sp->semaphores);
        free(sp);
    }
}

/**************************************************************************************************\
\**************************************************************************************************/
static NV_Semaphore nv_sem_pool_get_sem(NV_SemaphorePool spool) {
    uint32_t sem_idx;

    if (spool) {
        HAL_NV_CRITICAL_SECTION_DECLARE;
        HAL_NV_CRITICAL_SECTION_BEGIN();
        for (sem_idx = 0; sem_idx < spool->no_sems; sem_idx++) {
            if (!spool->semaphores[sem_idx].pool) {
                spool->semaphores[sem_idx].pool = spool;
                HAL_NV_CRITICAL_SECTION_END();
                return &spool->semaphores[sem_idx];
            }
        }
        HAL_NV_CRITICAL_SECTION_END();
    }

    return NULL;
}

/**************************************************************************************************\
\**************************************************************************************************/
static void nv_sem_pool_return_sem(NV_Semaphore sem) {
    HAL_NV_CRITICAL_SECTION_DECLARE;
    HAL_NV_CRITICAL_SECTION_BEGIN();
    if (sem && sem->pool) {
        sem->pool = NULL;
    }
    HAL_NV_CRITICAL_SECTION_END();
}

/**************************************************************************************************\
\**************************************************************************************************/
static NV_RequestQueue nv_request_queue_create(uint32_t qlen) {
    NV_RequestQueue retval;

    retval = NULL;

    if (qlen) {
        retval = malloc(sizeof(NV_RequestQueue_T));

        if (retval) {
            retval->requests = malloc(qlen * sizeof(NV_Request_T));
            if (retval->requests) {
                retval->max_no_requests = qlen;
                retval->pending_requests = 0;
                retval->head = 0;
                retval->tail = 0;
            } else {
                free(retval);
                retval = NULL;
            }
        }
    }

    return retval;
}

/**************************************************************************************************\
\**************************************************************************************************/
static void nv_request_queue_delete(NV_RequestQueue rq) {
    if (rq) {
        free(rq->requests);
        free(rq);
    }
}

/**************************************************************************************************\
\**************************************************************************************************/
static NV_OpResult nv_add_request(NV_RequestQueue q, NV_Memory dev, NV_OpType type, NV_Addressable addr, NV_Addressable size, void *ptr,
        NV_Semaphore notification, volatile NV_OpResult *result) {
    NV_OpResult retval;
    NV_Request req;

    HAL_NV_CRITICAL_SECTION_DECLARE;
    HAL_NV_CRITICAL_SECTION_BEGIN();

    req = nv_allocate_request(q);

    if (req) {
        req->dev = dev;
        req->op_type = type;
        req->nv_addr = addr;
        if (NV_READ == type) {
            req->data.mem_ptr = ptr;
        } else {
            req->data.op_buf = ptr;
        }
        req->length = size;
        req->notification = notification;
        req->result = result;
        retval = NVOP_OK;
    } else {
        retval = NVOP_TOO_MANY_REQ;
    }
    HAL_NV_CRITICAL_SECTION_END();

#if defined(HAL_NV_USE_SEM_TO_PROCESS_IDLE) && (HAL_NV_USE_SEM_TO_PROCESS_IDLE != 0)
    if (req)
    {
        HAL_NV_WAIT_SEM_POST(dev->parent_dev->req_queue_sem);
    }
    #endif

    return retval;
}

/**************************************************************************************************\
\**************************************************************************************************/
static void nv_process_request(NV_Request req) {
    NV_OpResult result;

    if (req) {
        switch (req->op_type) {
            case NV_NOP:
                break;

            case NV_READ:
                result = req->dev->ops->read(req->dev, req->nv_addr, req->length, req->data.mem_ptr);
                NV_ASSIGN_RESULT(req->result, result);
                HAL_NV_SEMAPHORE_GIVE(req->notification->sem);
                break;

            case NV_ASYNC_WRITE:
            case NV_SYNC_WRITE:
                nv_process_write_request(req);
                break;

            case NV_ERASE:
                result = req->dev->ops->erase(req->dev);
                *(req->result) = result;
                HAL_NV_SEMAPHORE_GIVE(req->notification->sem);
                break;

            case NV_FLUSH:
                *(req->result) = NVOP_OK;
                HAL_NV_SEMAPHORE_GIVE(req->notification->sem);
                break;

            default:
                HAL_NV_CRITICAL_ERROR();
                break;
        }
    }
}

#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)*/

/**************************************************************************************************\
\**************************************************************************************************/
static void nv_memory_init(NV_Memory mdev, NV_MemDevice parent) {
    if (mdev && parent) {
        mdev->parent_dev = parent;
        if (mdev->ops && mdev->ops->init) {
            mdev->ops->init(mdev);
        }
    }

    if (!mdev || !parent || !mdev->ops) {
        HAL_NV_CRITICAL_ERROR();
    }
}

/**************************************************************************************************\
\**************************************************************************************************/
static void nv_memory_deinit(NV_Memory mdev) {
    if (mdev) {
        if (mdev->ops && mdev->ops->deinit) {
            mdev->ops->deinit(mdev);
        }
        mdev->parent_dev = NULL;
    }

    if (!mdev || !mdev->ops) {
        HAL_NV_CRITICAL_ERROR();
    }
}

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)

/**************************************************************************************************\
\**************************************************************************************************/
static NV_Request nv_allocate_request(NV_RequestQueue q) {
    NV_Request retval;

    retval = NULL;

    if (q->max_no_requests > q->pending_requests) {
        q->pending_requests++;
        retval = &q->requests[q->tail];
        q->tail++;
        if (q->tail == q->max_no_requests) {
            q->tail = 0;
        }
    }

    return retval;
}

/**************************************************************************************************\
\**************************************************************************************************/
static NV_Request nv_get_request(NV_RequestQueue q) {
    NV_Request retval;

    retval = NULL;

    if (q->pending_requests) {
        q->pending_requests--;
        retval = &q->requests[q->head];
        q->head++;
        if (q->head == q->max_no_requests) {
            q->head = 0;
        }
    }

    return retval;
}

#endif /* defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0) */

/**************************************************************************************************\
\**************************************************************************************************/
static void nv_process_write_request(NV_Request req) {
    uint32_t src_offset;
    uint32_t wr_size;
    NV_Addressable wr_addr;
    NV_Addressable nv_addr;
    uint32_t len;
    NV_OpResult result;
    uint8_t *buf;

    len = req->length;
    buf = req->dev->parent_dev->page_buffer;
    nv_addr = req->nv_addr;
    src_offset = 0;

    do {
        wr_size = nv_get_block_at(req->dev->mem_map, nv_addr, &wr_addr);

        if ((nv_addr != wr_addr) || (len < wr_size)) {
            result = req->dev->ops->read(req->dev, wr_addr, wr_size, buf);
            if (result != NVOP_OK) {
                break;
            }
        }

        wr_size -= nv_addr - wr_addr;
        wr_size = (len > wr_size) ? wr_size : len;
        (void) BP_CopyToMem(req->data.op_buf, buf + (nv_addr - wr_addr), src_offset, wr_size);

        result = req->dev->ops->write(req->dev, wr_addr, buf);

        src_offset += wr_size;
        nv_addr += wr_size;
        len -= wr_size;
    } while ((NVOP_OK == result) && (len > 0));

#if defined(HAL_NV_USE_WORKER_TASK) && (HAL_NV_USE_WORKER_TASK != 0)
    if (req->notification) {
        // synchronous request
        // set result before notification
        NV_ASSIGN_RESULT(req->result, result);
        // notify requester
        HAL_NV_SEMAPHORE_GIVE(req->notification->sem);
    } else {
        // asynchronous request - free resources
        BP_ReleaseBuffer(req->data.op_buf);
        // set result after resources are freed
        NV_ASSIGN_RESULT(req->result, result);
    }
#else
    NV_ASSIGN_RESULT(req->result, result);
#endif
}

#undef HAL_NV_C_
