/*
 * @file hal_heap.h
 * @brief HAL heap memory manager
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_HEAP_H
#define HAL_HEAP_H

#include <stdint.h>
#include <stdlib.h>

#include "hal_config.h"

/** \defgroup hal_heap HEAP module
 *
 * <b>Introduction.</b>
 *
 * The heap memory manager provides simple dynamic memory allocation mechanisms for embedded systems.
 * It was introduced to fight the chaos of heap management.
 *
 * <b>Background.</b>
 *
 * A heap is simply a dedicated pool of memory, on which the program can allocate (reserve) some space
 * for further use. The allocation is done dynamically at runtime, thus it's often called dynamic
 * memory. When the allocated memory on the heap is not needed anymore, the program should free it
 * (cancel reservation, deallocate). This is true for most PC applications. In embedded systems
 * freeing heap memory is often not needed and even depreciated, because of the memory fragmentation
 * effect and allocation time uncertainty.
 *
 * The default and most portable dynamic memory management is provided by standard C functions:
 * malloc() and free(). These are often used in more complex embedded systems, but for low-complex
 * or highly resource/time-constrained applications, the default implementations are often too
 * time and resource consuming. Fortunately, in most embedded systems the C library implementation
 * allow the user to alter the default behavior of malloc() and free(), so that the dynamic memory
 * management is done the right, application-specific way. Unfortunately this approach is not so
 * portable and depends on the particular C library implementation used.
 *
 * Because of these drawbacks, some embedded real-time operating systems have their own memory managers.
 * A good example is the FreeRTOS system, were heap management is done via pvPortMalloc and vPortFree
 * portable functions. The only drawback, is that when there are is more than one heap memory manager
 * working in a single application, there is also more than one heap space. And most often the heap
 * space is oversized due to the fact that it is hard to define the exact maximum amount of heap needed.
 * So in case of more stack space, the memory waste increases and it is more complex to track several
 * heaps (for example checking overflows) than just one.
 *
 * To make based applications less vulnerable to heap problems, a dedicated heap engine
 * has been introduced.
 *
 * <b>Heap engine.</b>
 *
 * The heap memory is allocated by a call to <b>HEAP_Alloc()</b> and freed by a call to <b>HEAP_Free()</b>.
 * This is simple and straightforward. The external behavior of these functions should be identical
 * to malloc() and free().
 *
 * <b>Heap modes.</b>
 *
 * The heap engine is controlled by the HAL_HEAP_MODE definition in hal_config.h. These modes control
 * the internal behavior of HEAP_Alloc() and HEAP_Free(). There are 4 modes of operation defined:
 *
 * <i>MODE 0:</i>
 *
 * This is the default mode, the most passive and "uninvasive". In this mode internal
 * heap memory manager is disabled (does not compile). The heap will be serviced by standard
 * malloc() and free() implementation, unless there is a default OS implementation (like pvPortMalloc
 * and vPortFree for FreeRTOS). Use this mode if you don't know what else to do.
 *
 * <i>MODE 1:</i>
 *
 * In this mode the heap will be serviced by a simple internal heap manager. This also means, that
 * whenever possible, OS heap will also be serviced by this internal heap manager.
 * Remember that the simple internal memory manager does NOT provide methods for freeing allocated heap space!
 * In this mode additional functions: \ref HEAP_GetSpaceUsed and \ref HEAP_GetSpaceLeft are available
 * to check how much space is used and how much space is left on the heap.
 * This is the most recommended mode for applications.
 *
 * <i>MODE 2:</i>
 *
 * In this mode the heap will be serviced by more advanced internal heap manager. This also means, that
 * whenever possible, OS heap will also be serviced by this internal heap manager.
 * This internal memory manager provide methods for both allocating and freeing heap space.
 * In this mode additional functions: \ref HEAP_GetSpaceUsed and \ref HEAP_GetSpaceLeft are available
 * to check how much space is used and how much space is left on the heap.
 *
 * <i>MODE 3:</i>
 *
 * This is the user-defined mode. In this mode, the user is responsible for providing HEAP_Alloc()
 * and HEAP_Free() implementations.
 *
 * <b>Size of heap.</b>
 *
 * In modes 1 and 2 the heap size is defined by HAL_HEAP_SIZE definition in hal_config.h.
 * In mode 0 the size of the heap is defined in other way, specific to the C library used.
 *
 * <b>Memory blocks alignment.</b>
 *
 * It is possible to align every block allocated on the heap to a number of bytes, although it only
 * works in MODE 1. To do that, the HAL_HEAP_ALIGNMENT must be defined. HAL_HEAP_ALIGNMENT equal to
 * zero or one will have no effect on the alignment. By default the blocks on the heap are aligned
 * to four bytes (HAL_HEAP_ALIGNMENT equal to 4).
 *
 * <b>Tracing memory allocation.</b>
 *
 * In MODE 1, the heap manager will throw diagnostic messages through the DIAG module on every
 * allocation, when HAL_HEAP_DEBUG definition is present and equal to one.
 *
 */
/*@{*/

#ifndef HAL_HEAP_MODE
#define HAL_HEAP_MODE       0
#endif

#ifndef HAL_HEAP_DEBUG
#define HAL_HEAP_DEBUG      0
#endif

#ifndef HAL_HEAP_ALIGNMENT
#define HAL_HEAP_ALIGNMENT  4
#endif

#if defined HAL_HEAP_MODE && (HAL_HEAP_MODE == 0)

// In this mode the heap is serviced by malloc() and free(), with exception to
// OS-specific heap management (such as FreeRTOS pvPortMalloc and vPortFree)

#define HEAP_Alloc      malloc

#define HEAP_Free       free

#endif

#if defined HAL_HEAP_MODE && ((HAL_HEAP_MODE == 1) || (HAL_HEAP_MODE == 2))

// In this mode the heap is serviced by simple internal memory manager

void* HEAP_Alloc(size_t size);

void HEAP_Free(void *ptr);

size_t HEAP_GetSpaceUsed(void);

size_t HEAP_GetSpaceLeft(void);

#endif

#if defined HAL_HEAP_MODE && (HAL_HEAP_MODE == 3)

// In this mode the the user is responsible for providing own implementation
// of HEAP_Alloc and HEAP_Free functions.

extern void* HEAP_Alloc(size_t size);

extern void HEAP_Free(void *ptr);

#endif

/*@}*/

#endif /* HAL_HEAP_H */
