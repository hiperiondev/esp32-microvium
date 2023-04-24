/*
 * @file hal_heap1.c
 * @brief HAL heap memory manager
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#if defined HAL_HEAP_MODE && HAL_HEAP_MODE == 1
#include <string.h>
#include <stdint.h>

#include "hal_core.h"
#include "hal_heap.h"
#include "hal_diag.h"

#ifndef HAL_HEAP_SIZE
#error HAL_HEAP_SIZE was not defined. Check your hal_config.h !
#endif

#if HAL_HEAP_SIZE <= 0
#error HAL_HEAP_SIZE must be greater than zero! Check your hal_config.h !
#endif

// heap buffer, the struct is used to make sure the buffer is properly aligned
static struct HAL_HEAP {
    unsigned long dummy;
    uint8_t pool[HAL_HEAP_SIZE];
} HAL_Heap;

// heap index
static size_t HAL_HeapIndex = 0;

// -----------------------------------------------------------------------------
//  HEAP_Alloc
// -----------------------------------------------------------------------------
void* HEAP_Alloc(size_t size) {
    void *ptr;

#if defined HAL_HEAP_DEBUG && (HAL_HEAP_DEBUG > 0)
    DIAG_LogMsg("HEAP alloc ");
    DIAG_LogINT(size, 10);
#endif

#if defined HAL_HEAP_ALIGNMENT && ((HAL_HEAP_ALIGNMENT == 0) || (HAL_HEAP_ALIGNMENT == 1))
    // align to 1 byte means no alignment
#elif defined HAL_HEAP_ALIGNMENT && (HAL_HEAP_ALIGNMENT == 2)
    // align to 2 bytes
    if (size & 0x1) {
        size += 1;
    }
#elif defined HAL_HEAP_ALIGNMENT && (HAL_HEAP_ALIGNMENT == 4)
    // align to 4 bytes
    if (size & 0x3) {
        size += 4 - (size & 0x3);
    }
#elif defined HAL_HEAP_ALIGNMENT && (HAL_HEAP_ALIGNMENT == 8)
    // align to 8 bytes
    if (size & 0x7) {
        size += 8 - (size & 0x7);
    }
#elif defined HAL_HEAP_ALIGNMENT
#error Wrong HAL_HEAP_ALIGNMENT definition. Check your hal_config.h !
#error Possible values for HAL_HEAP_ALIGNMENT: 0, 1, 2, 4, 8
#endif

    CORE_EnterCritical();

    if (HAL_HeapIndex + size < HAL_HEAP_SIZE) {
        ptr = (void*) (HAL_Heap.pool + HAL_HeapIndex);
        HAL_HeapIndex += size;

        CORE_ExitCritical();

#if defined HAL_HEAP_DEBUG && (HAL_HEAP_DEBUG > 0)
        DIAG_LogMsg(" OK, left ");
        DIAG_LogINT(HAL_HEAP_SIZE - HAL_HeapIndex, 10);
        DIAG_LogMsg(" bytes.");
        DIAG_LogNL();
#endif
        return ptr;
    }

    CORE_ExitCritical();

#if defined HAL_HEAP_DEBUG && (HAL_HEAP_DEBUG > 0)
    DIAG_LogMsg(" failed! Only ");
    DIAG_LogINT(HAL_HEAP_SIZE - HAL_HeapIndex, 10);
    DIAG_LogMsg(" bytes available.");
    DIAG_LogNL();
#endif

    return NULL;

} /* HEAP_Alloc */

// -----------------------------------------------------------------------------
//  HEAP_Free
// -----------------------------------------------------------------------------
void HEAP_Free(void *ptr) {

    return;

} /* HEAP_Free */

// -----------------------------------------------------------------------------
//  HEAP_GetSpaceUsed
// -----------------------------------------------------------------------------
size_t HEAP_GetSpaceUsed(void) {
    return HAL_HeapIndex;

} /* HEAP_GetSpaceUsed */

// -----------------------------------------------------------------------------
//  HEAP_GetSpaceLeft
// -----------------------------------------------------------------------------
size_t HEAP_GetSpaceLeft(void) {
    return HAL_HEAP_SIZE - HAL_HeapIndex;

} /* HEAP_GetSpaceLeft */

#endif // HAL_HEAP_MODE == 1
