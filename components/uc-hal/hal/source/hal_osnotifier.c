/*
 * @file hal_osnotifier.c
 * @brief HAL OSNotifier
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include <stdlib.h>

#include "hal_osnotifier.h"
#include "hal_os.h"
#include "hal_core.h"

// -----------------------------------------------------------------------------
//  OSNTF_Create
// -----------------------------------------------------------------------------
OSNotifier OSNTF_Create(OSSem sem) {
    OSNotifier notifier;

    notifier = (OSNotifier) malloc(sizeof(OSNotifierDesc));
    if (!notifier) {
        // memory allocation problem!
        return NULL;
    }

    if (sem) {
        notifier->os_sem = sem;
    } else {
    notifier->os_sem = OSSEM_Create();
}

return notifier;

} /* OSNTF_Create */

// -----------------------------------------------------------------------------
//  OSNTF_Destroy
// -----------------------------------------------------------------------------
void OSNTF_Destroy(OSNotifier notifier) {
if (notifier) {
    OSSEM_Destroy(notifier->os_sem);
    free(notifier);
}
} /* OSNTF_Destroy */

// -----------------------------------------------------------------------------
//  OSNTF_WaitForData
// -----------------------------------------------------------------------------
int OSNTF_WaitForData(OSNotifier notifier, size_t data_count, uint32_t timeout) {
if (notifier) {
    CORE_EnterCritical();
    notifier->data_counter = data_count;
    if (OSSEM_Take(notifier->os_sem, 0)) {
        ;
    }
    CORE_ExitCritical();
    if (OSSEM_Take(notifier->os_sem, timeout)) {
        return 1;
    }
    OSSEM_Give(notifier->os_sem);
}
return 0;
} /* OSNTF_WaitForData */
