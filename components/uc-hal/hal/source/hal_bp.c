/*
 * @file hal_bp.c
 * @brief Definition of the buffer pool interface.
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

#include "hal_bp.h"

typedef struct BP_BufferPool_Tag {
    HAL_BP_MUTEX_DECLARE(access_lock);
    struct BP_PartialBuf_Tag *buffers;
    uint32_t avail_buffers;
    uint32_t no_buffers;
    uint32_t buf_len;
} BP_BufferPool_T;

// -----------------------------------------------------------------------------
//  BP_Create
// -----------------------------------------------------------------------------
BP_BufferPool BP_Create(uint32_t no_buffers, uint32_t buffer_size) {
    BP_BufferPool bp;
    uint8_t *mem_area;
    uint32_t buf_idx;

    if (!no_buffers || !buffer_size) {
        return NULL;
    }

    bp = HEAP_Alloc(sizeof(BP_BufferPool_T));

    if (bp) {
        bp->buffers = HEAP_Alloc(no_buffers * sizeof(BP_PartialBuf_T));

        if (bp->buffers) {
            // adjust buffer size to multiple of the HAL_BP_MEM_ALIGN
            buffer_size = HAL_BP_MEM_ALIGN * ((buffer_size + (HAL_BP_MEM_ALIGN - 1)) / HAL_BP_MEM_ALIGN);
            // allocate memory for the all created buffer as one chunk
            mem_area = HEAP_Alloc(no_buffers * buffer_size);

            if (mem_area) {
                memset(mem_area, 0, no_buffers * buffer_size);
                bp->avail_buffers = no_buffers;
                bp->no_buffers = no_buffers;
                bp->buf_len = buffer_size;
                HAL_BP_MUTEX_CREATE(bp->access_lock);
                for (buf_idx = 0; buf_idx < no_buffers; buf_idx++) {
                    bp->buffers[buf_idx].data = mem_area;
                    bp->buffers[buf_idx].next = NULL;
                    bp->buffers[buf_idx].pool = NULL;
                    bp->buffers[buf_idx].size = buffer_size;
                    mem_area += buffer_size;
                }
            } else {
                HEAP_Free(bp->buffers);
                HEAP_Free(bp);
                bp = NULL;
            }
        } else {
            HEAP_Free(bp);
            bp = NULL;
        }
    }

    return bp;

} /* BP_Create */

// -----------------------------------------------------------------------------
//  BP_GetBuffer
// -----------------------------------------------------------------------------
BP_PartialBuffer BP_GetBuffer(BP_BufferPool bp, uint32_t buf_len) {
    uint32_t buf_idx;
    uint32_t bufs_needed;
    BP_PartialBuffer ob;
    BP_PartialBuffer prev;

    ob = NULL;

    if (bp && buf_len) {
        bufs_needed = (buf_len + (bp->buf_len - 1UL)) / bp->buf_len;
        if (bp->no_buffers >= bufs_needed) {
            HAL_BP_MUTEX_LOCK(bp->access_lock);
            if (bp->avail_buffers >= bufs_needed) {
                bp->avail_buffers -= bufs_needed;
                for (buf_idx = 0; bufs_needed; buf_idx++) {
                    if (!bp->buffers[buf_idx].pool) {
                        bp->buffers[buf_idx].pool = bp;
                        if (!ob) {
                            ob = &bp->buffers[buf_idx];
                        } else {
                            prev->next = &bp->buffers[buf_idx];
                        }
                        prev = &bp->buffers[buf_idx];
                        bufs_needed--;
                    }
                }
                // mark buffer in the chain as the latest one
                prev->next = NULL;
                ob->size = buf_len;
            }
            HAL_BP_MUTEX_UNLOCK(bp->access_lock);
        }
    }

    return ob;

} /* BP_GetBuffer */

// -----------------------------------------------------------------------------
//  BP_ReleaseBuffer
// -----------------------------------------------------------------------------
void BP_ReleaseBuffer(BP_PartialBuffer buf) {
    if (buf && buf->pool) {
        HAL_BP_MUTEX_LOCK(buf->pool->access_lock);
        do {
            buf->pool->avail_buffers++;
            memset(buf->data, 0, buf->pool->buf_len);
            buf->pool = NULL;
            buf = buf->next;
        } while (buf);
        HAL_BP_MUTEX_UNLOCK(buf->pool->access_lock);
    }
} /* BP_ReleaseBuffer */

// -----------------------------------------------------------------------------
//  BP_CopyToMem
// -----------------------------------------------------------------------------
uint32_t BP_CopyToMem(BP_PartialBuffer src, void *dst, uint32_t src_offset, uint32_t data_len) {
    uint32_t buf_offset;
    uint32_t copy_size;
    uint32_t retval;
    uint32_t single_buf_len;

    retval = 0;

    if (src && dst && data_len && (src_offset < src->size)) {
        if ((src_offset + data_len) > src->size) {
            data_len = src->size - src_offset;
        }
        retval = data_len;

        single_buf_len = src->pool ? src->pool->buf_len : src->size;

        // find first buffer with begin of the requested data (at specified offset)
        for (buf_offset = single_buf_len; src_offset >= buf_offset; buf_offset += single_buf_len) {
            src = src->next;
        }

        // copy first data chunk
        buf_offset -= single_buf_len;
        buf_offset = src_offset - buf_offset;
        copy_size = single_buf_len - buf_offset;
        copy_size = (copy_size <= data_len) ? copy_size : data_len;
        memcpy(dst, ((const uint8_t*) src->data) + buf_offset, copy_size);
        data_len -= copy_size;

        // copy the rest of data
        while (data_len) {
            dst = (uint8_t*) dst + copy_size;
            src = src->next;
            copy_size = (single_buf_len < data_len) ? single_buf_len : data_len;
            memcpy(dst, src->data, copy_size);
            data_len -= copy_size;
        }
    }

    return retval;

} /* BP_CopyToMem */

// -----------------------------------------------------------------------------
//  BP_CopyToBuf
// -----------------------------------------------------------------------------
uint32_t BP_CopyToBuf(BP_PartialBuffer dst, const void *src, uint32_t dst_offset, uint32_t data_len) {
    uint32_t single_buf_len;
    uint32_t buf_offset;
    uint32_t copy_size;
    uint32_t retval;

    retval = 0;

    if (src && dst && data_len && (dst_offset < dst->size)) {
        single_buf_len = dst->pool ? dst->pool->buf_len : dst->size;

        if ((dst_offset + data_len) > dst->size) {
            data_len = dst->size - dst_offset;
        }
        retval = data_len;

        // find first buffer with begin of the requested data (at specified offset)
        for (buf_offset = single_buf_len; dst_offset >= buf_offset; buf_offset += single_buf_len) {
            dst = dst->next;
        }

        // copy first data chunk
        buf_offset -= single_buf_len;
        buf_offset = dst_offset - buf_offset;
        copy_size = single_buf_len - buf_offset;
        copy_size = (copy_size <= data_len) ? copy_size : data_len;
        memcpy(((uint8_t*) dst->data) + buf_offset, src, copy_size);
        data_len -= copy_size;

        // copy the rest of data
        while (data_len) {
            src = (uint8_t*) src + copy_size;
            dst = dst->next;
            copy_size = (single_buf_len < data_len) ? single_buf_len : data_len;
            memcpy(dst->data, src, copy_size);
            data_len -= copy_size;
        }
    }

    return retval;

} /* BP_CopyToBuf */

// -----------------------------------------------------------------------------
//  BP_InitStandaloneBuf
// -----------------------------------------------------------------------------
void BP_InitStandaloneBuf(BP_PartialBuffer buf, void *mem_area, uint32_t mem_size) {
    if (buf && mem_area && mem_size) {
        buf->data = mem_area;
        buf->next = NULL;
        buf->pool = NULL;
        buf->size = mem_size;
    }
} /* BP_InitStandaloneBuf */
