/*
 * @file hal_osnotifier.h
 * @brief HAL OSNotifier API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_OS_NOTIFIER_H
#define HAL_OS_NOTIFIER_H

#include <stdint.h>

#include "hal_os.h"

/** 
 * \defgroup hal_osnotifier OSNotifier interface
 * 
 * A simple notifier used in I/O operations. The implementation is OS-independent
 * and relies completely on \ref hal_os_sem.
 */
/*@{*/

/** OSNotifier descriptor */
typedef struct {
    size_t data_counter;
    OSSem os_sem;
} OSNotifierDesc, *OSNotifier;

// -----------------------------------------------------------------------------
//  PUBLIC MACROS
// -----------------------------------------------------------------------------

/** 
 *  @param notifier
 *  @param count
 */
#define OSNTF_DataTick(notifier, count)                     do { \
                                                                if (notifier->data_counter <= count) { \
                                                                    notifier->data_counter = 0; \
                                                                    OSSEM_Give(notifier->os_sem); \
                                                                } else { \
                                                                    notifier->data_counter -= count; \
                                                                } \
                                                            } while (0)

/** 
 *  @param notifier
 */
#define OSNTF_ForceNotification(notifier)                   do { OSSEM_Give(notifier->os_sem); } while (0)

// -----------------------------------------------------------------------------
//  PUBLIC API
// -----------------------------------------------------------------------------

/** 
 *  Creates a new instance of OSNotifier
 *  
 *  @param sem pointer to the OSSem semaphore that will be associated with the new
 *  OSNotifier. If this is NULL, the OSSem will be created automatically 
 */
OSNotifier OSNTF_Create(OSSem sem);

/** 
 *  Realeases an instance of the OSNotifier
 *  
 *  @param notifier
 */
void OSNTF_Destroy(OSNotifier notifier);

/** 
 *  Halts current task waiting for data. Returns 0 if the notification occured before timeout, 
 *  and 1 if timeout occured.
 *  @param notifier
 *  @param timeout
 *  @param data_count
 */
int OSNTF_WaitForData(OSNotifier notifier, size_t data_count, uint32_t timeout);

/*@}*/
#endif /* HAL_OS_NOTIFIER_H */
