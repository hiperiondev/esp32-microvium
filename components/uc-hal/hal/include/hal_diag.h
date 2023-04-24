/*
 * @file hal_diag.h
 * @brief HAL diagnostic module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_DIAG_H
#define HAL_DIAG_H

#include <stdint.h>

#include "hal_iodevice.h"
#include "hal_config.h"
#if defined HAL_ENABLE_OS && (HAL_ENABLE_OS > 0)
#include "hal_os.h"
#endif

/** \defgroup hal_diag DIAG module
 * 
 * <b>Introduction.</b>
 * 
 * The diagnostics module in HAL (called DIAG) provides a standard way for user 
 * modules and applications to report their condition. The DIAG module supports:
 *
 *   - creating log output with simple text formatting tools to the specified IODevice
 *   - standarized way to signal and manage errors, that are reported from multiple 
 *     points in an application
 *   - assertions executed depending on the configuration (debug or release)
 *
 * The DIAG module is also used internally by other HAL modules, to report their 
 * errors and log status.
 * 
 * <b>Status logging.</b>
 * 
 * The status logging is particularly useful during development stage. The DIAG 
 * module API provides several functions to make the log output formating easier. 
 * These functions are: DIAG_LogChar, DIAG_LogMsg, DIAG_LogINT, DIAG_LogUINT 
 * and DIAG_LogNL (see the HAL API reference for more info about using these 
 * functions). The log output is directed to the IODevice, associated with the 
 * DIAG module, with a prior call to DIAG_SetOutputDevice.This IODevice must be 
 * properly configured and operational.
 * 
 * <b>Assertions.</b>
 * 
 * The diagnostics module provides four macros for making assertions in the 
 * program flow. Assertions are simply true-false statements used to express, that 
 * some conditions during code execution are always met. If they're not then something 
 * is wrong. The DIAG module provides these macros for handling assertions: 
 * DIAG_DEBUG_ASSERT(), DIAG_RELEASE_ASSERT(), DIAG_DEBUG_ASSERT_AND_EXECUTE() and
 * DIAG_RELEASE_ASSERT_AND_EXECUTE()
 * 
 * The DIAG_DEBUG_ASSERT() and DIAG_DEBUG_ASSERT_AND_EXECUTE() assertions are checked 
 * ONLY when the DIAG module is configured  to compile in DEBUG configuration. This 
 * configuration is set by a macro  HAL_DIAG_DEBUG_LEVEL in hal_config.h. See "Module 
 * configuration" section below for details. 
 * The DIAG_RELEASE_ASSERT() and DIAG_RELEASE_ASSERT_AND_EXECUTE() assertions are checked 
 * ALWAYS. It means that the programmer can do additional checking through the 
 * DIAG_DEBUG_ASSERT() macro, that won't be executed in the release configuration. 
 * Consider the following example:
 * 
 * \code
 * int divide(int x, int y)
 * {
 *     DIAG_RELEASE_ASSERT(y != 0);
 *     DIAG_DEBUG_ASSERT(x > y);
 *     return x/y;
 * }
 * \endcode
 * 
 * In the DEBUG configuration the following function will report violations when y is 
 * equal to zero and when x is less or equal to y. In the RELEASE configuration, only 
 * the first violation is checked. However, even though the violation is reported, the 
 * division in the example above is still executed, which can lead to unwanted behavior.
 * To protect execution of a code block, under assertion condition use 
 * DIAG_DEBUG_ASSERT_AND_EXECUTE() or DIAG_RELEASE_ASSERT_AND_EXECUTE() macros. Let's 
 * modify the example above:
 * 
 * \code
 * int divide(int x, int y)
 * {
 *     DIAG_RELEASE_ASSERT_AND_EXECUTE(y != 0) {
 *         DIAG_DEBUG_ASSERT(x > y) {
 *             return x/y;
 *         }
 *     }
 *     return 0;
 * }
 * \endcode
 * 
 * Now, the division is only executed when y is not equal to 0. If y is 0, an error is reported
 * and the following code block is not executed. The division is never executed also, when x <= y
 * whether the HAL_DIAG_DEBUG_LEVEL is set to DEBUG configuration or not. The only difference is
 * that when it is actually set to DEBUG, an error is reported, when x <= y.
 * 
 * <b>Managing errors.</b>
 * 
 * The error handling capability of the DIAG module can be configured and tuned to 
 * cover a wide range of applications, from simple to quite complex ones.The whole 
 * error management idea is built around one function: DIAG_ReportError that is called 
 * in the error signaling module, and provides the one and only method for reporting
 * an error. A single error report consists of several items:
 * 
 *   - ID of the calling module,
 *   - error code (error number),
 *   - line of code from which the error notification was called
 * 
 * Optionally, the report can be extended with (see module configuration section below):
 * 
 *   - the time of notification
 *   - error description string
 *   - additional user supplied data
 * 
 * The error report can be processed right away - in the context of the signalling 
 * module, or it can be buffered and processed later by a call to DIAG_ProcessErrors. 
 * The specification of this behavior is configured at compile-time through definitions. 
 * See module configuration section below for more details.
 * 
 * <b>Processing a single error.</b>
 * 
 * No matter whether the error buffering feature is enabled or not, processing an 
 * error consists of several steps. First, the DIAG module tries to call the 
 * application's error handler, set with a prior call to DIAG_SetErrorHandler. 
 * If this handler is present, it is called and it's return value decides, if the 
 * error should be logged to the output device (return !0) or not (return 0). 
 * If the error is to be logged, and valid IODevice is set as an output device, 
 * the second application handler is called, to get the literal description of the 
 * error. This handler can be set by a prior call to DIAG_SetErrorDescriptionProvider. 
 * Finally the information about an error is written to the output device. If buffering 
 * is enabled, then the processed error is removed from the queue.
 *
 * An error can be printed out to IODevice in two ways. When HAL_DIAG_ERROR_SEND_AS_FORMATED_DATA
 * is set to 1, the error is printed in a readable form. If it is set to 0, then error
 * descriptor is printed in a binary form (to simplify machine-to-machine communications).
 * 
 * <b>Using DIAG module in multi-task software.</b>
 * 
 * When HAL_ENABLE_OS is set to 1 in hal_config.h, the DIAG module integrates with the OS module
 * providing access controll to output IODevice. This guard relies on IODevice lock/unlock mechanisms.
 * The following example illustrates how to use this feature:
 *
 * \code
 * if (0 == DIAG_Lock(100)) {
 *     DIAG_LogMsg("This is a diagnostic message.");
 *     DIAG_LogMsg("Hopefully it won't get split in half.");
 *     DIAG_Unlock();
 * }
 * \endcode 
 *
 * In the example above, the diagnostic message won't be displayed if the lock is not successful.
 * We can use ASSERT statement to reveal such cases:
 *
 * \code
 * DIAG_RELEASE_ASSERT_AND_EXECUTE(0 == DIAG_Lock(100)) {
 *     DIAG_LogMsg("This is a diagnostic message.");
 *     DIAG_LogMsg("Hopefully it won't get split in half.");
 *     DIAG_Unlock();
 * }
 * \endcode 
 *
 * It is advised to use this strategy in multi-tasking (OS-based) software for all DIAG-based
 * output, to prevent undesired output conflicts. Please note, that the error reporting feature
 * already uses this mechanism. In this case additional definition: \ref HAL_DIAG_LOCK_TIMEOUT
 * is required to configure the timeout, when waiting for IODevice lock. The default timeout 
 * value is 1 second.
 *
 * <b>Module configuration.</b>
 * 
 * To enable the DIAG module, HAL_ENABLE_DIAG definition must be set to 1, 
 * in hal_config.h. To switch between the DEBUG and RELEASE configurations use 
 * HAL_DIAG_DEBUG_LEVEL definition. 0 means DEBUG and 1 means RELEASE. The default
 * configuration is DEBUG. 
 * 
 * To enable error buffering, the HAL_DIAG_USE_ERROR_BUFFERING must be set to 1. 
 * The HAL_DIAG_ERROR_BUFFER_SIZE defines how many errors can fit into error buffer.
 * To enable buffering, it must be set to a non-zero value. 
 * 
 * To extend error report with a timestamp, set HAL_DIAG_USE_ERROR_TIME_STAMPS to 1.
 * To extend error report with an error description, set HAL_DIAG_USE_ERROR_DESCRIPTIONS to 1.
 * 
 */
/*@{*/

// -----------------------------------------------------------------------------
//  GLOBAL MACROS
// -----------------------------------------------------------------------------
#ifndef HAL_DIAG_DEBUG_LEVEL
// 0 - DEBUG CONFIGURATION
// 1 - RELEASE CONFIGURATION
#define HAL_DIAG_DEBUG_LEVEL                        0
#endif

#if ((HAL_ENABLE_DIAG == 1) && (HAL_DIAG_DEBUG_LEVEL == 0))
/** Tests the assertion condition. Works ONLY in DEBUG mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_DEBUG_ASSERT(condition)                if (!(condition)) do { \
                                                        DIAG_LogMsg("Assertion failed in "); \
                                                        DIAG_LogMsg(__FILE__); \
                                                        DIAG_LogMsg(" at line "); \
                                                        DIAG_LogUINT(__LINE__, 10); \
                                                        DIAG_LogNL(); \
                                                    } while (0)

/** Tests the assertion condition and executes the following code block, if the 
 * condition is true. Works ONLY in DEBUG mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_DEBUG_ASSERT_AND_EXECUTE(condition)    if (!(condition)) { \
                                                        DIAG_LogMsg("Assertion failed in "); \
                                                        DIAG_LogMsg(__FILE__); \
                                                        DIAG_LogMsg(" at line "); \
                                                        DIAG_LogUINT(__LINE__, 10); \
                                                        DIAG_LogNL(); \
                                                    } else

#else
/** Tests the assertion condition. Works ONLY in DEBUG mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_DEBUG_ASSERT(condition) 

/** Tests the assertion condition and executes the following code block, if the 
 * condition is true. Works ONLY in DEBUG mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_DEBUG_ASSERT_AND_EXECUTE(condition)    if (condition)
#endif

#if (HAL_ENABLE_DIAG == 1)
/** Tests the assertion condition. Works both in DEBUG and RELEASE mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_RELEASE_ASSERT(condition)              if (!(condition)) do { \
                                                        DIAG_LogMsg("Assertion failed in "); \
                                                        DIAG_LogMsg(__FILE__); \
                                                        DIAG_LogMsg(" at line "); \
                                                        DIAG_LogUINT(__LINE__, 10); \
                                                        DIAG_LogNL(); \
                                                    } while (0)

/** Tests the assertion condition and executes the following code block, if the 
 * condition is true. Works both in DEBUG and RELEASE mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_RELEASE_ASSERT_AND_EXECUTE(condition)  if (!(condition)) { \
                                                        DIAG_LogMsg("Assertion failed in "); \
                                                        DIAG_LogMsg(__FILE__); \
                                                        DIAG_LogMsg(" at line "); \
                                                        DIAG_LogUINT(__LINE__, 10); \
                                                        DIAG_LogNL(); \
                                                    } else
#else
/** Tests the assertion condition. Works both in DEBUG and RELEASE mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_RELEASE_ASSERT(condition)  

/** Tests the assertion condition and executes the following code block, if the 
 * condition is true. Works both in DEBUG and RELEASE mode! 
 * @param condition expression to evaluate. This must be true to pass the assertion test.
 */
#define DIAG_RELEASE_ASSERT_AND_EXECUTE(condition)  if (condition)
#endif

#define DIAG_Lock(timeout)                          IODEV_Lock(DIAG_GetOutputDevice(), timeout)

#define DIAG_Unlock()                               IODEV_Unlock(DIAG_GetOutputDevice());

#ifndef HAL_DIAG_USE_ERROR_BUFFERING
/// Enable error buffering by default. 0 disables error buffering capabilities od the DIAG module
#define HAL_DIAG_USE_ERROR_BUFFERING                1
#endif

#ifndef HAL_DIAG_ERROR_BUFFER_SIZE
/// This definition controlls how may error descriptions can fit into error buffer, when error buffering is enabled. Must be greater than 0
#define HAL_DIAG_ERROR_BUFFER_SIZE                  30
#endif

#ifndef HAL_DIAG_USE_ERROR_DESCRIPTIONS
/// Enables (1) or disables (0) error descriptions
#define HAL_DIAG_USE_ERROR_DESCRIPTIONS             1
#endif

#ifndef HAL_DIAG_USE_ERROR_TIME_STAMPS
/// Enables (1) or disables (0) error time stamps
#define HAL_DIAG_USE_ERROR_TIME_STAMPS              1
#endif

#ifndef HAL_DIAG_ERROR_SEND_AS_FORMATED_DATA
/// Set to 1 to enable error time stamps
#define HAL_DIAG_ERROR_SEND_AS_FORMATED_DATA        1
#endif

#ifndef HAL_DIAG_LOCK_TIMEOUT
/// Set to default 1s timeout
#define HAL_DIAG_LOCK_TIMEOUT                       1000
#endif

#ifndef HAL_DIAG_NL_MODE
#define HAL_DIAG_NL_MODE                            0
#endif

// -----------------------------------------------------------------------------
//  API
// -----------------------------------------------------------------------------

/** 
 * Initializes the diagnostics module
 */
void DIAG_Init(void);

/** 
 * Deinitializes the diagnostics module
 */
void DIAG_Deinit(void);

/** 
 *  Sets the output device for all diagnostic messages and error logs. Setting this
 *  to NULL disables all output, but not the error processing itself.
 *  
 *  @param iodevice iodevice used for diagnostic output
 */
void DIAG_SetOutputDevice(IODevice iodevice);

/** 
 *  Gets the currently set output device.
 *  
 *  @return IODevice currently used for diagnostic output
 */
IODevice DIAG_GetOutputDevice(void);

/** 
 *  Logs a single character
 *  
 *  @param character character that will be written to the output device
 */
void DIAG_LogChar(char character);

/** 
 *  Logs a string message
 *  
 *  @param msg string message that will be written to the output device
 */
void DIAG_LogMsg(const char *msg);

/** 
 *  Logs a signed integer value
 *  
 *  @param value signed value that will be written to the output device
 *  @param base a base for integer to string conversion
 */
void DIAG_LogINT(int32_t value, uint8_t base);

/** 
 *  Logs an unsigned integer value
 *  
 *  @param value unsigned value that will be written to the output device
 *  @param base a base for integer to string conversion
 */
void DIAG_LogUINT(uint32_t value, uint8_t base);

/** 
 * Logs an end-of-line character
 */
void DIAG_LogNL(void);

/** 
 *  Reports an error
 *  
 *  @param module_id id of the calling module
 *  @param error_no error number
 *  @param code_line line of code from which the notification was called
 *  @param user_data user supplied data attached to the error
 *  @param description pointer to the error description string
 */
void DIAG_ReportError(uint16_t module_id, uint16_t error_no, uint32_t code_line, void *user_data, char *description);

/** 
 *  A parser function that handles the buffered errors. Used when error buffering
 *  for DIAG is enabled. This function should be called periodically by the
 *  application to ensure proper error handling.
 *  @return number of errors still left to parse
 *  
 *  @param max_error_count specifies the maximum number of errors that can be processed
 *                         in one run of this function
 */
uint32_t DIAG_ProcessErrors(uint32_t max_error_count);

/** 
 *  Sets an error handler callback function, that will be called when an error is
 *  signalled. 
 */
void DIAG_SetErrorHandler(int (*error_handler)(uint16_t, uint16_t, uint16_t, void*, const char*));

/** 
 *  Sets an error description provider callback function, that will be called when
 *  an error is signalled, to get the literal string description for an error. This
 *  description will be written to the specified output iodevice. 
 */
void DIAG_SetErrorDescriptionProvider(char* (*error_desc_provider)(uint16_t, uint16_t, uint16_t, void*, const char*));

/**
 *  Prints out some basic information about an IODevice in human readable form.
 *  @param iodevice IODevice to print
 *  @param indent indentation of log (number of spaces)
 */
void DIAG_PrintIODeviceInfo(IODevice iodevice, int indent);

/**
 *  Prints out some basic information about an IOBuf in human readable form.
 *  @param iobuf IOBuf to print
 *  @param indent indentation of log (number of spaces)
 */
void DIAG_PrintIOBufInfo(IOBuf iobuf, int indent);

/*@}*/

#endif /* HAL_DIAG_H */
