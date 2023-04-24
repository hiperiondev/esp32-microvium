/*
 * @file hal_bp.h
 * @brief HAL buffer pool module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_BP_H_
#define HAL_BP_H_

#include <stdint.h>

#include "hal_config.h"
#include "hal_core.h"

/** \defgroup hal_bp BP module
 *
 * <b>Introduction.</b>
 *
 * The BP module offers memory pool functionality with additional feature allowing
 * to treat many buffers as a single memory chunk (with fully linear and continuous access).
 * Because "allocation" and "deallocation" memory from the buffer (memory) pool has
 * known the worst execution time then this kind of pseudo dynamic memory allocation
 * is allowed to use in the real time environment.
 * Additionally to allow consistent usage of buffers (instead of raw pointers to memory)
 * it is added functionality to create a stand-alone buffer wrapping preallocated memory.
 *
 * <b>Buffers.</b>
 * Allocated buffers have constant (usually small) size but the single "allocation"
 * (the BP_GetBuffer function) can be made up to total size of allocated
 * memory (buffer_size * no_of_buffers).
 * When accessed by the BP_CopyTo functions the buffer returned by the BP_GetBuffer function can
 * treated as a single continuous memory area.
 * The "deallocation" procedure requires only single call to the BP_ReleaseBuffer function.
 *
 * <b>BP setup.</b>
 *
 * There is no special setup required for this module.
 * The user should take into account that BP_Create function uses a real dynamic allocation
 * functions and thus it should be avoided when real time tasks are already started.
 * The recommended way of buffer pool creation is to do it before RTOS scheduler is executed.
 *
 * <b>Using the BP module</b>
 *
 * \code
 * uint8_t mem_chunk[512];
 * BP_BufferPool nv_buf_pool;
 * nv_buf_pool = BP_Create(8, 256);
 * BP_PartialBuffer buf;
 * buf = BP_GetBuffer(buf_pool, sizeof(mem_chunk));
 * (void) BP_CopyToBuf(buf, mem_chunk, 0, 300);
 * (void) BP_CopyToMem(buf, mem_chunk, 128, 256);
 * BP_ReleaseBuffer(buf);
 * \endcode
 *
 * The example of stand-alone buffer usage.
 *
 * <b>Module configuration.</b>
 *
 * \code
 * uint8_t mem_chunk[1024];
 * BP_PartialBuf_T buf;
 * BP_InitStandaloneBuf(&buf, mem_chunk, sizeof(mem_chunk));
 * (void) BP_CopyToBuf(&buf, some_mem_ptr, 0, 300);
 * \endcode
 *
 * To enable the NV module, HAL_ENABLE_BP definition must be set to 1,
 * in hal_config.h.
 */

/*
 * Internal configuration of the buffer pool module.
 */

/* The alignment of the memory area inside of the (partial) buffers. */
#define HAL_BP_MEM_ALIGN                4UL

#define HAL_BP_MUTEX_DECLARE(name)      void* name
#define HAL_BP_MUTEX_CREATE(name)       name = NULL
#define HAL_BP_MUTEX_LOCK(name)         CORE_EnterCritical()
#define HAL_BP_MUTEX_UNLOCK(name)       CORE_ExitCritical()

struct BP_BufferPool_Tag;
typedef struct BP_BufferPool_Tag *BP_BufferPool;

typedef struct BP_PartialBuf_Tag {
    uint32_t size;
    void *data;
    struct BP_PartialBuf_Tag *next;
    BP_BufferPool pool;
} BP_PartialBuf_T;

typedef BP_PartialBuf_T *BP_PartialBuffer;

/**
 *  Creates the BP_BufferPool object.
 *  This functions dynamically (using malloc) allocates memory for
 *  the buffer pool object and for managed buffers as well.
 *
 *  @param no_buffers   the number of buffers associated with given buffer pool object
 *  @param buffer_size  the size of a single buffer
 *
 *  @return             when allocation is not possible then it is NULL, otherwise it is pointer to created buffer pool
 */
BP_BufferPool BP_Create(uint32_t no_buffers, uint32_t buffer_size);

/**
 *  Allocates memory using buffers from the BP_BufferPool object.
 *  This functions allocates requested amount of memory using preallocated memory chunks (partial buffers) from the BP_BufferPool object.
 *  All further accesses to the allocated memory should be made using API functions from this module.
 *  When allocated memory is no longer needed then it should be released using \c BP_ReleaseBuffer function.
 *
 *  @param bp       the BP_BufferPool object
 *  @param buf_len  the size of requested memory
 *
 *  @return         when allocation is not possible then it is NULL, otherwise it is pointer to partial buffer
 */
BP_PartialBuffer BP_GetBuffer(BP_BufferPool bp, uint32_t buf_len);

/**
 *  Releases memory allocated previously from the BP_BufferPool object.
 *  This functions returns back allocated memory to the parent BP_BufferPool object.
 *  After this call \c buf object shall not be used anymore.
 *
 *  @param buf      the partial buffer object returned by the \c BP_GetBuffer function
 */
void BP_ReleaseBuffer(BP_PartialBuffer buf);

/**
 *  Copy data from buffer pool memory to user memory.
 *  This functions copies data_len bytes from memory allocated in buffer pool into specified RAM location.
 *  The copy operation is performed starting from (supplied as parameter) offset. This allow to use buffer pool
 *  memory as it was continuous.
 *
 *  @param src          the partial buffer object returned by the \c BP_GetBuffer function
 *  @param dst          the pointer to the memory area
 *  @param src_offset   the starting offset (in bytes) in the source (buffer pool memory)
 *  @param data_len     the length (in bytes) of the requested copy operation
 *
 *  @return             the number of copied bytes
 */
uint32_t BP_CopyToMem(BP_PartialBuffer src, void *dst, uint32_t src_offset, uint32_t data_len);

/**
 *  Copy data from user memory to buffer pool memory.
 *  This functions copies data_len bytes from RAM location into specified memory allocated in buffer pool.
 *  The copy operation is performed starting from (supplied as parameter) offset. This allow to use buffer pool
 *  memory as it was continuous.
 *
 *  @param dst          a partial buffer object returned by the \c BP_GetBuffer function
 *  @param src          a pointer to the memory area
 *  @param dst_offset   a starting offset (in bytes) in the destination (buffer pool memory)
 *  @param data_len     a length (in bytes) of the requested copy operation
 *
 *  @return             a number of copied bytes
 */
uint32_t BP_CopyToBuf(BP_PartialBuffer dst, const void *src, uint32_t dst_offset, uint32_t data_len);

/**
 *  Initializes the BP_PartialBuffer_T object as a stand-alone object (not associated with buffer pool).
 *
 *  @param buf          a pointer to the partial buffer object, it has to be already allocated (e.g. as a local variable)
 *  @param mem_area     a pointer to the (already allocated) memory area
 *  @param mem_size     a size of the memory area
 */
void BP_InitStandaloneBuf(BP_PartialBuffer buf, void *mem_area, uint32_t mem_size);

#endif /* HAL_BP_H_ */
