/*
 * @file hal_heap2.c
 * @brief HAL heap memory manager
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#if defined HAL_HEAP_MODE && HAL_HEAP_MODE == 2
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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

#define BLOCK_SIZE              64
typedef uint32_t BlockSizeAddOp_T;
typedef uint32_t BlockSize_T;
#define BlockSize_T_BITS        32

#define RESERVED_BIT        (1UL << (BlockSize_T_BITS - 1))
#define TO_BLOCK_PTR(p)     (Block_T*)((BlockSize_T*) (p) - 1)
#define BLOCKS_TO_BYTES(n)  ((((n) & ~RESERVED_BIT) * BLOCK_SIZE) - sizeof(BlockSize_T))

//-----------------------------------------------------------------------------
// A memory block is made up of a header and either a user data part or a pair
// of pointers to the previous and next free area. A memory area is made up
// of one or more of these blocks. Only the first block will have the header.
// The header contains the number of blocks contained within the area.
//
typedef struct Block_Tag {
    BlockSize_T blocks;
    union {
        uint8_t user_memory[BLOCK_SIZE - sizeof(BlockSize_T)];
        struct {
            struct Block_Tag *prev;
            struct Block_Tag *next;
        } ptrs;
    } header;
} Block_T;

//-----------------------------------------------------------------------------
// Local global variables.
//
static Block_T *mem_pool;
static Block_T *sentinel;
static BlockSize_T number_of_blocks;

static void init_mem_pool(void);
static void merge_blocks(Block_T *block);

//-----------------------------------------------------------------------------
//
static inline void insert_after(Block_T *block) {
    Block_T *p;
    p = sentinel->header.ptrs.next;
    sentinel->header.ptrs.next = block;
    block->header.ptrs.prev = sentinel;
    block->header.ptrs.next = p;
    p->header.ptrs.prev = block;
}

//-----------------------------------------------------------------------------
//
static inline void unlink(Block_T *block) {
    block->header.ptrs.prev->header.ptrs.next = block->header.ptrs.next;
    block->header.ptrs.next->header.ptrs.prev = block->header.ptrs.prev;
}

//-----------------------------------------------------------------------------
//
static inline void split_block(Block_T *block, uint32_t blocks_required) {
    Block_T *new_block;
    new_block = block + blocks_required;            // create a remainder area
    new_block->blocks = block->blocks - blocks_required;   // set its size and mark as free
    block->blocks = blocks_required;                      // set us to requested size
    insert_after(new_block);                          // stitch remainder into free list
}

//-----------------------------------------------------------------------------
// Compute the number of blocks that will fit in the memory area defined.
// Allocate the pool of blocks. Note this includes the sentinel area that is 
// attached to the end and is always only one block. The first entry in the 
// free list pool is set to include all available blocks. The sentinel is 
// initialised to point back to the start of the pool.
//
static void init_mem_pool(void) {
    mem_pool = (Block_T*) (HAL_Heap.pool);
    number_of_blocks = ((BlockSize_T) ((HAL_Heap.pool + HAL_HEAP_SIZE) - (HAL_Heap.pool)) / (BlockSize_T) BLOCK_SIZE) - (BlockSize_T) 1UL;
    sentinel = mem_pool + number_of_blocks;

    sentinel->blocks = RESERVED_BIT;           // now cannot be used
    sentinel->header.ptrs.prev = sentinel;
    sentinel->header.ptrs.next = sentinel;

    // Entire pool is initially a single unallocated area.
    //
    mem_pool[0].blocks = number_of_blocks;    // initially all of free memeory
    insert_after(&mem_pool[0]);               // link the sentinel
}

//-----------------------------------------------------------------------------
// As each area is examined for a fit, we also examine the following area. 
// If it is free then it must also be on the Free list. Being a doubly-linked 
// list, we can combine these two areas in constant time. If an area is 
// combined, the procedure then looks again at the following area, thus 
// repeatedly combining areas until a reserved area is found. In terminal 
// cases this will be the sentinel block.
//
static void merge_blocks(Block_T *block) {
    Block_T *successor;

    successor = block + block->blocks;

    while (0 == (successor->blocks & RESERVED_BIT)) {
        unlink(successor);
        block->blocks += successor->blocks;
        successor = block + block->blocks;
    }

#if 0
    while (true)
    {
        successor = block + block->blocks;          // point to next area
        if (successor->blocks & RESERVED_BIT)       // done if reserved
        {
            //return block;
            break;
        }
        unlink(successor);
        block->blocks += successor->blocks;         // add in its blocks
    }
#endif
}

// -----------------------------------------------------------------------------
//  HEAP_Alloc
// -----------------------------------------------------------------------------
void* HEAP_Alloc(size_t size) {
    BlockSize_T blocks_required;
    Block_T *block;

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

    // Compute the number of blocks to satisfy the request.
    blocks_required = ((BlockSizeAddOp_T) size + (BlockSizeAddOp_T) sizeof(BlockSize_T) + (BlockSizeAddOp_T) sizeof(Block_T) - 1)
            / (BlockSizeAddOp_T) BLOCK_SIZE;

    CORE_EnterCritical();

    // Initialise the heap for a first-time use.
    //
    if (NULL == sentinel) {
        init_mem_pool();
    }

    // Traverse the free list looking for the first block that will fit the
    // request. This is a "first-fit" strategy.
    //
    for (block = sentinel->header.ptrs.next; block != sentinel; block = block->header.ptrs.next) {
        merge_blocks(block);
        // Is this free area (which could have just expanded somewhat)
        // large enough to satisfy the request.
        if (block->blocks >= blocks_required) {
            break;
        }
    }

    // If we are pointing at the sentinel then all blocks are allocated.
    //
    if (block == sentinel) {
        CORE_ExitCritical();
#if defined HAL_HEAP_DEBUG && (HAL_HEAP_DEBUG > 0)
    DIAG_LogMsg(" failed! Only ");
    DIAG_LogINT(0, 10);
    DIAG_LogMsg(" bytes available.");
    DIAG_LogNL();
#endif
        return NULL;
    }

    // If this free area is larger than required, it is split in two. The
    // size of the first area is set to that required and the second area
    // to the blocks remaining. The second area is then inserted into 
    // the free list.
    //
    if (block->blocks > blocks_required) {
        split_block(block, blocks_required);
    }

    // unlink the now correctly sized area from the free list and mark it 
    // as reserved.
    //
    unlink(block);
    block->blocks |= RESERVED_BIT;

    CORE_ExitCritical();

#if defined HAL_HEAP_DEBUG && (HAL_HEAP_DEBUG > 0)
        DIAG_LogMsg(" OK, left ");
        DIAG_LogINT(0, 10);
        DIAG_LogMsg(" bytes.");
        DIAG_LogNL();
#endif  

    return block->header.user_memory;

} /* HEAP_Alloc */

// -----------------------------------------------------------------------------
//  HEAP_Free
// -----------------------------------------------------------------------------
void HEAP_Free(void *ptr) {
    Block_T *top;
    Block_T *base_area;

    if (NULL == ptr) {
        return;
    }

    top = mem_pool + number_of_blocks;
    base_area = TO_BLOCK_PTR(ptr);

    if ((base_area < mem_pool) || (base_area >= top)) {
        return;
    }

    // Very simple - we insert the area to be freed at the start
    // of the free list. This runs in constant time. Since the free
    // list is not kept sorted, there is less of a tendency for small
    // areas to accumulate at the head of the free list.
    //
    CORE_EnterCritical();
    base_area->blocks &= ~RESERVED_BIT;
    insert_after(base_area);
    CORE_ExitCritical();

} /* HEAP_Free */

// -----------------------------------------------------------------------------
//  HEAP_GetSpaceUsed
// -----------------------------------------------------------------------------
size_t HEAP_GetSpaceUsed(void) {
    return 0;

} /* HEAP_GetSpaceUsed */

// -----------------------------------------------------------------------------
//  HEAP_GetSpaceLeft
// -----------------------------------------------------------------------------
size_t HEAP_GetSpaceLeft(void) {
    return 0;

} /* HEAP_GetSpaceLeft */

#endif // HAL_HEAP_MODE == 2
