/*
 * @file hal_iobuf.h
 * @brief HAL IOBuf module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_IOBUF_H
#define HAL_IOBUF_H

#include <stdint.h>
#include <stdlib.h>

/** \defgroup hal_iobuf IOBuf objects
 * 
 * 
 * <b>Introduction.</b>
 * 
 * The IOBuf is a simple implementation of a ring buffer. It is used by several
 * HAL modules and can also be used as a general-purpose buffer by the application.
 * 
 * <b>Inside IOBuf.</b>
 * 
 * The IOBuf consists of a descriptor and memory space (pool) used for storing data.
 * The descriptor holds all the information needed for IOBuf to operate. The IOBuf
 * stores <b>elements</b> of data. A signle element is an array of bytes. The bytesize of
 * an element is fixed for every IOBuf during initialization. Typical examples of 
 * elements are bytes (bytesize is simply one), but it could also be pointers or
 * structures.
 * 
 * <b>Using IOBufs.</b>
 * 
 * The IOBuf object must be declared or created at runtime by a call to \ref IOBUF_Create.
 * To declare IOBuf at compile time you must provide the memory pool. The buffer must be then
 * initialized by a call to \ref IOBUF_Init. The following code declares a buffer for
 * 2-byte elements. The IOBuf descriptor is placed on top of the memory pool, so the actual
 * memory pool size is sizeof(IOBufDesc) less. 
 * \code
 * char iobuf_pool[100];
 * IOBUF iobuf;
 * IOBUF_Init(iobuf_pool, sizeof(iobuf_pool), 2);
 * \endcode
 * To create IOBuf at runtime you need a dynamic memory manager (see also notes below).
 * Then all you need to do is call \ref IOBUF_Create. You DO NOT call \ref IOBUF_Init as the
 * buffer is already initialized. The following example allocates a buffer for maximum of 
 * 10 elements, each of bytesize 2.
 * \code
 * IOBUF iobuf;
 * iobuf = IOBUF_Create(2, 10);
 * \endcode
 * To destroy it you must call \ref IOBUF_Destroy.
 * \code
 * IOBUF_Destroy(iobuf);
 * \endcode
 * 
 * The buffer can be written by a call to \ref IOBUF_Write and read from by a call to
 * \ref IOBUF_Read. The \ref IOBUF_GetCount function returns the number of elements
 * currently stored in the buffer. The \ref IOBUF_GetSpace function returns the
 * number of elements that can still fit into the buffer.
 * 
 * 
 * <b>Dynamic memory and IOBufs.</b>
 * 
 * Creating and destroying dynamic IOBufs requires dynamic memory manager. By default HAL
 * calls malloc() and free() to do the task, but you can change that by overriding HAL_MEM_ALLOC
 * and HAL_MEM_FREE definitions in hal_config.h:
 * \code
 * #define HAL_MEM_ALLOC(size)    my_malloc(size)
 * #define HAL_MEM_FREE (ptr)     my_free(ptr)
 * \endcode
 * 
 */
/*@{*/

/**
 * IOBuf descriptor 
 */
typedef struct {
    /// Number of elements stored in a a buffer
    size_t elem_count;
    /// Size of the single element (in bytes)
    size_t elem_size;
    /// Pointer to the actual buffer pool memory area
    void *mem_ptr;
    /// Pointer to the next element to read
    void *read_ptr;
    /// Maximum number of elements that the buffer can store (logical size of the buffer)
    size_t max_elem_count;
    /// Pointer to the are where next element will be written
    void *write_ptr;
} IOBufDesc, *IOBuf;

/** 
 *  Creates a buffer. Allocates the memory for the buffer and its descriptor.
 *  @return handle of the created buffer or NULL, if there were errors.
 *  
 *  @param elem_size size in bytes of asingle element that can be stored in a
 *  buffer
 *  @param max_elem_count maximum number of elements that the buffer can store (size of
 *  bufer) 
 */
IOBuf IOBUF_Create(size_t elem_size, size_t max_elem_count);

/** 
 *  Destroys a buffer, freeing memory for the buffer pool and the descriptor.
 *  
 *  @param iobuf handle of the buffer to destroy
 */
IOBuf IOBUF_Destroy(IOBuf iobuf);

/** 
 *  Returns the number of elements stored in a buffer.
 *  
 *  @param iobuf handle of the buffer
 */
size_t IOBUF_GetCount(IOBuf iobuf);

/** 
 *  Returns the number of elements that can still be written to a buffer.
 *  @return actual number of elements that can be written or 0, if the buffer
 *  handle is invalid or buffer is full.
 *  
 *  @param iobuf handle of the buffer
 */
size_t IOBUF_GetSpace(IOBuf iobuf);

/** 
 *  Returns the total number of elements that the buffer can store (buffer capacity).
 *  @return buffer capacity
 *
 *  @param iobuf handle of the buffer
 */
size_t IOBUF_GetSize(IOBuf iobuf);

/**
 *  Initializes a buffer inside the specified memory area, allowing static memory
 *  allocation. This area must be big enough to hold the iobuf memory pool and the
 *  iobuf descriptor. The actual memory usage for the buffer is elem_size *
 *  buf_count + sizeof(IOBufDesc). The buffer descriptor is placed at the beginning
 *  of the specified memory area, so after successful initialization, the returned
 *  handle will be the same as buf_ptr.
 *  @return handle of a newly initialized buffer or NULL if initialization failed
 *  
 *  @param buf_ptr pointer to a memory area that will be used by the buffer
 *  @param buf_size size of the memory area
 *  @param elem_size size in bytes of a single element in the buffer
 */
IOBuf IOBUF_Init(void *buf_ptr, size_t buf_size, size_t elem_size);

/** 
 *  Reads a specified number of elements from a buffer. The data is copied
 *  from the buffer's internal memory pool to the destination space, pointed
 *  by the data parameter.
 *  @return number of elements actually read
 *  
 *  @param iobuf handle of the buffer
 *  @param data pointer to the destination space
 *  @param elem_count number of elements to read
 */
size_t IOBUF_Read(IOBuf iobuf, void *data, size_t elem_count);

/** 
 *  Writes a specified number of elements to a buffer. The data is copied
 *  from the provided source space to the buffer's internal memory pool.
 *  @return number of elements actually written
 *  
 *  @param iobuf handle of the buffer
 *  @param data pointer to the source data
 *  @param elem_count number of elements to write
 */
size_t IOBUF_Write(IOBuf iobuf, const void *data, size_t elem_count);

/** 
 *  Reads a portion of data from a buffer by returning a pointer to the next
 *  fragment of data. This function differs from \ref IOBUF_Read, because in
 *  this case there is no data copying. This function gives out a pointer
 *  to sequential memory blocks from the buffer's internal memory pool, and acts
 *  like the caller actualy read it (by progressing buffer's internal read mark).
 * 
 *  This function tries to read as much as possible at once. This means, that
 *  it will try to read max_fragment_size elements. However, this may not be 
 *  possible for two reasons:
 *  - the buffer stores less elements (obvious)
 *  - the requested data wraps around the buffer's internal memory pool (that's
 *  because of the ring-nature of the buffer)
 * 
 *  In the second case, the requested data is actually split in the memory, and
 *  no consistent memory block can be returned. Only the first data block will
 *  be returned, the second one will be read during the next function call,
 *  even though it stored in the buffer already.
 *  
 *  @param iobuf handle of the buffer
 *  @param fragment_size specifies the maximum amount of elements that can be
 *  read by this call (the requested data size)
 *  @param next_fragment_ptr pointer to a variable, where a pointer to the next
 *  portion of data to read will be stored
 *  @param next_fragment_size pointer to a variable, where the size of the next
 *  portion of data to read will be stored 
 */
size_t IOBUF_ReadNextFragment(IOBuf iobuf, size_t fragment_size, void **next_fragment_ptr, size_t *next_fragment_size);

/** 
 *  Allows to write data directly to the buffer's internal memory pool avoiding
 *  in some cases the unnecessary data copying, while still operating in the
 *  ring-buffer fashion. This function gets a pointer and size of the memory
 *  block and acts as it was actually written (by progressing buffer's internal 
 *  write mark). By using this function, the caller actually requests sequential
 *  blocks of space in the ring-buffer.
 *  
 *  This function tries to get as much space as possible at once. This means, that
 *  it will try to get space for fragment_size elements. However, this may not be 
 *  possible for two reasons:
 *  - the buffer capacity is exceeded (obvious)
 *  - the requested data space wraps around the buffer's internal memory pool (that's
 *  because of the ring-nature of the buffer)
 * 
 *  In the second case, the requested data space is actually split in the memory, and
 *  no consistent memory block can be requested. Only the first data block will
 *  be returned, the second one will be returned during the next function call,
 *  even though it may be ready in the buffer.
 * 
 *  @param iobuf handle of the buffer
 *  @param fragment_size specifies the amount of elements that the caller wants to
 *  write to the buffer
 *  @param next_fragment_ptr pointer to a variable, where a pointer to the next
 *  available data chunk will be stored
 *  @param next_fragment_size pointer to a variable, where the size of the
 *  available data chunk will be stored. This can be less or equal to the
 *  fragment_size parameter. 
 */
size_t IOBUF_WriteNextFragment(IOBuf iobuf, size_t fragment_size, void **next_fragment_ptr, size_t *next_fragment_size);

/**
 *  Clears the content stored in the buffer, making it return to initial state.
 *  @param iobuf handle of the buffer
 */
void IOBUF_Clear(IOBuf iobuf);

/** 
 *  Reads a specified number of elements from a buffer, but without altering the 
 *  state of the buffer. The data read is not removed from the buffer. This means that
 *  two sequential calls will return the same data.
 *  The data is copied from the buffer's internal memory pool to the destination space, 
 *  pointed by the data parameter. 
 *  @return number of elements actually read
 *  
 *  @param iobuf handle of the buffer
 *  @param data pointer to the destination space
 *  @param elem_count number of elements to read
 */
size_t IOBUF_Peek(IOBuf iobuf, void *data, size_t elem_count);

/**
 *  Moves a specified number of elements from the source buffer into the destination buffer.
 *  It may fail to move the desired number of elements due to:
 *  - destination buffer overrun
 *  - source buffer underrun
 *  In any of the above cases, this function will move as much data as possible.
 * 
 *  @return number of elements actually moved
 *  
 *  @param dst_iobuf handle of the destination buffer
 *  @param src_iobuf handle of the source buffer
 *  @param elem_count number of elements to move
 */
size_t IOBUF_Move(IOBuf dst_iobuf, IOBuf src_iobuf, size_t elem_count);

/**
 *  Removes a specified number of elements from a buffer. The data is removed
 *  from the buffer's internal memory pool and discarded.
 *  @return number of elements actually removed
 *
 *  @param iobuf handle of the buffer
 *  @param elem_count number of elements to remove
 */
size_t IOBUF_Remove(IOBuf iobuf, size_t elem_count);

/*@}*/

#endif /* hal_iobuf_h */

