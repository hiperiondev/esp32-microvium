/*
 * @file hal_os.h
 * @brief HAL OS module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_OS_H
#define HAL_OS_H

#include "hal_port_os.h"

/** \defgroup hal_os OS module
 * 
 * <b>Introduction.</b>
 * 
 * The OS module in HAL provides an abstraction of most common operating system 
 * mechanisms. It is used mainly to allow HAL to use some OS functionality in so called
 * OS INTEGRATION mode. The application may also use OS module for better OS-portability.
 * 
 * <b>Interfaces.</b>
 * 
 * At the moment, the OS module provides interfaces for:
 * - tasks (see \ref hal_os_task)
 * - binary semaphores (see \ref hal_os_sem)
 * - counting semaphores (see \ref hal_os_cntsem)
 * - mutual exclusion semaphores (see \ref hal_os_mutex)
 * 
 */
/*@{*/

/**
 * Starts the OS scheduler.
 */
#define OS_Start()              OS_PORT_Start()

/**
 * Stops the OS scheduler.
 */
#define OS_Stop()               OS_PORT_Stop()

/**
 * Puts the calling task to sleep for a number of ticks
 * @param time time (in miliseconds) after which the task will be woken up again
 */
#define OS_Sleep(time)          OS_PORT_Sleep(time)

/**
 * Puts the calling task to sleep until specified time is reached
 * @param time absolute time at which the calling task will be woken up again
 */
#define OS_SleepUntil(time)     OS_PORT_SleepUntil(time)

/**
 * Returns the current system time (usually the number of system ticks that elapsed 
 * since reset). 
 */
#define OS_GetSystemTime()      OS_PORT_GetSystemTime()

/*@}*/

// -----------------------------------------------------------------------------
//  TASK API
// -----------------------------------------------------------------------------
/** \defgroup hal_os_task OSTask interface
 * 
 *  An interface to a task mechanism provided by the OS. The implementation of 
 *  the OSTask depends on the OS used.
 */
/*@{*/

/** 
 *  Creates a new task. The implementation depends on the actual OS.
 *  @returns handle of a newly created task
 *  
 *  @param task_impl pointer to the task implementation function
 *  @param priority task priority
 *  @param stack_size stack size (in bytes)
 *  @param arg task argument
 */
#define OSTASK_Create(task_impl, priority, stack_size, arg)     OSTASK_PORT_Create(task_impl, priority, stack_size, arg)

/** 
 *  Destroys a task
 *  
 *  @param task handle of the task to destroy
 */
#define OSTASK_Destroy(task)                    OSTASK_PORT_Destroy(task)

/** 
 *  Suspends the execution of a task
 *  
 *  @param task handle of the task to suspend
 */
#define OSTASK_Suspend(task)                    OSTASK_PORT_Suspend(task)

/** 
 *  Resumes the execution of a task
 *  
 *  @param task handle of the task to resume
 */
#define OSTASK_Resume(task)                     OSTASK_PORT_Resume(task)

/**
 *  Causes the calling task to relinquish the CPU
 */
#define OSTASK_Yield()                          OSTASK_PORT_Yield()

/** 
 *  Gets the priority of a task.
 *  @return task priority
 *  
 *  @param task handle of the task
 */
#define OSTASK_GetPriority(task)                OSTASK_PORT_GetPriority(task)

/** 
 *  Sets the priority of a task
 *  
 *  @param task handle of the task
 *  @param priority new task priority
 */
#define OSTASK_SetPriority(task, priority)      OSTASK_PORT_SetPriority(task, priority)

/**
 *  Retrieves the currently running task
 *  @return handle of the currently running task
 */
#define OSTASK_GetCurrentTask()                 OSTASK_PORT_GetCurrentTask()

/*@}*/

// -----------------------------------------------------------------------------
//  BINARY SEMAPHORE API
// -----------------------------------------------------------------------------
/** \defgroup hal_os_sem OSSem interface
 * 
 *  An interface to a binary semaphore mechanisms provided by the OS. The implementation
 *  of the OSSem depends on the OS used.
 */
/*@{*/

/** 
 *  Creates a new binary semaphore instance. The semaphore implementation depends on the
 *  actual OS.
 * 
 *  @returns a newly created binary semaphore
 */
#define OSSEM_Create()                      OSSEM_PORT_Create()

/**
 *  Destroys a binary semaphore
 *  
 *  @param sem binary semaphore to destroy
 */
#define OSSEM_Destroy(sem)                  OSSEM_PORT_Destroy(sem)

/** 
 *  Gives the binary semaphore.
 *  
 *  @param sem binary semaphore to give
 */
#define OSSEM_Give(sem)                     OSSEM_PORT_Give(sem)

/**
 *  Takes the binary semaphore. If the semaphore is not immediately available, the task
 *  from which the call was made is blocked. The maximum time spent waiting for the
 *  semaphore is specified by the timeout parameter.
 * 
 *  @param sem binary semaphore to take
 *  @param timeout maximum time spent waiting for the semaphore in miliseconds
 *  @return 0 if the semaphore was taken
 *          !0 if timeout or error occurred
 *  
 */
#define OSSEM_Take(sem, timeout)            OSSEM_PORT_Take(sem, timeout)

/*@}*/

// -----------------------------------------------------------------------------
//  COUNTING SEMAPHORE API
// -----------------------------------------------------------------------------
/** \defgroup hal_os_cntsem OSCntSem interface
 *
 *  An interface to a counting semaphore mechanisms provided by the OS. The implementation
 *  of the OSCntSem depends on the OS used.
 */
/*@{*/

/**
 *  Creates a new counting semaphore instance. The semaphore implementation depends on the
 *  actual OS.
 *
 *  @param init initial value of the counting semaphore
 *  @param max maximum value of the counting semaphore
 *  @returns a newly created binary semaphore
 */
#define OSCNTSEM_Create(init, max)          OSCNTSEM_PORT_Create(init, max)

/**
 *  Destroys a counting semaphore
 *
 *  @param sem counting semaphore to destroy
 */
#define OSCNTSEM_Destroy(sem)               OSCNTSEM_PORT_Destroy(sem)

/**
 *  Increments the value of a counting semaphore.
 *
 *  @param sem counting semaphore to give
 */
#define OSCNTSEM_Give(sem)                  OSCNTSEM_PORT_Give(sem)

/**
 *  Takes the counting semaphore. If the semaphore is not immediately available, the task
 *  from which the call was made is blocked. The maximum time spent waiting for the
 *  semaphore is specified by the timeout parameter.
 *
 *  @param sem counting semaphore to take
 *  @param timeout maximum time spent waiting for the semaphore in miliseconds
 *  @return 0 if the semaphore was taken
 *          !0 if timeout or error occurred
 *
 */
#define OSCNTSEM_Take(sem, timeout)         OSCNTSEM_PORT_Take(sem, timeout)

/*@}*/

// -----------------------------------------------------------------------------
//  MUTUAL EXCLUSION SEMAPHORE (MUTEX) API
// -----------------------------------------------------------------------------

/** \defgroup hal_os_mutex OSMutex interface
 *
 *  An interface to a mutual exclusion semaphore (mutex) mechanism provided by the OS.
 *  The implementation of the OSMutex depends on the OS used.
 */
/*@{*/

/**
 *  Creates a new mutex instance. The mutex implementation depends on the
 *  actual OS.
 *
 *  @returns a newly created mutex
 */
#define OSMUTEX_Create()                    OSMUTEX_PORT_Create()

/**
 *  Destroys a mutex
 *
 *  @param mutex  mutex to destroy
 */
#define OSMUTEX_Destroy(mutex)              OSMUTEX_PORT_Destroy(mutex)

/**
 *  Gives the mutex.
 *
 *  @param mutex mutex to give
 */
#define OSMUTEX_Give(mutex)                 OSMUTEX_PORT_Give(mutex)

/**
 *  Takes the mutex. If the mutex is not immediately available, the task
 *  from which the call was made is blocked. The maximum time spent waiting for the
 *  mutex is specified by the timeout parameter.
 *
 *  @param mutex mutex to take
 *  @param timeout maximum time spent waiting for the mutex in miliseconds
 *  @return 0 if the mutex was taken
 *          !0 if timeout or error occurred
 *
 */
#define OSMUTEX_Take(mutex, timeout)        OSMUTEX_PORT_Take(mutex, timeout)

/*@}*/

#endif // HAL_OS_H
