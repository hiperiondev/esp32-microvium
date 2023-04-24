/*
 * @file hal_diag.c
 * @brief HAL diagnostic module
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

#include "hal_diag.h"
#include "hal_core.h"
#include "hal_iobuf.h"

#if (defined HAL_ENABLE_OS) && (HAL_ENABLE_OS)
#include "hal_os.h"
#endif

/**
 * An error descriptor structure
 */
typedef struct {
    /// id of the calling module
    uint16_t module_id;
    /// error number
    uint16_t error_no;
    /// line of code from which the notification was called
    uint32_t code_line;
    /// user supplied data attached to the error
    void *user_data;
#if(HAL_DIAG_USE_ERROR_TIME_STAMPS)
    /// time at which the condition ocured
    uint64_t time;
#endif
    /// pointer to the error description string
    char *description;
} ErrorDesc;

/** 
 *  \param module_id
 *  \param error_no
 *  \param code_line
 *  \param user_data
 *  \param description 
 */
char* (*DIAG_error_desc_provider)(uint16_t module_id, uint16_t error_no, uint16_t code_line, void *user_data, const char *description);

/** 
 *  \param module_id
 *  \param error_no
 *  \param code_line
 *  \param user_data
 *  \param description 
 */
int (*DIAG_error_handler)(uint16_t module_id, uint16_t error_no, uint16_t code_line, void *user_data, const char *description);

// IODevice used for DIAG output
static IODevice DIAG_OutputDevice;
#if defined HAL_ENABLE_OS && (HAL_ENABLE_OS > 0)
// Guarding mutex for output IODevice
OSMutex DIAG_OutputDeviceGuard;
#endif

// handy buffer
static char buffLogUINT[32];

#if (HAL_DIAG_USE_ERROR_BUFFERING)
// stuff used only in BUFFERED DIAG mode

#if(HAL_DIAG_USE_ERROR_DESCRIPTIONS)
static const char ErrBufFullMsg[] = { "Error buffer full - possible overflow incomming" };
#endif // HAL_DIAG_USE_ERROR_DESCRIPTIONS

// -----------------------------------------------------------------------------
//  error buffer
// -----------------------------------------------------------------------------
static uint8_t ErrorTable[((HAL_DIAG_ERROR_BUFFER_SIZE + 1) * sizeof(ErrorDesc)) + sizeof(IOBufDesc)];
static IOBuf ErrorBuf;

#endif // HAL_DIAG_USE_ERROR_BUFFERING

#define DIAG_INDENT(var, indent)            for (var=0; var< indent; var++) { \
                                                DIAG_LogChar(' '); \
                                            }

// -----------------------------------------------------------------------------
//  DIAG_Init
// -----------------------------------------------------------------------------
void DIAG_Init(void) {
#if (HAL_DIAG_USE_ERROR_BUFFERING)
    ErrorBuf = IOBUF_Init(ErrorTable, sizeof(ErrorTable), sizeof(ErrorDesc));
#endif
    DIAG_error_handler = NULL;
    DIAG_error_desc_provider = NULL;
#if defined HAL_ENABLE_OS && (HAL_ENABLE_OS > 0)
    DIAG_OutputDeviceGuard = OSMUTEX_Create();
#endif
} /* DIAG_Init */

// -----------------------------------------------------------------------------
//  DIAG_Deinit
// -----------------------------------------------------------------------------
void DIAG_Deinit(void) {
    DIAG_error_handler = NULL;
    DIAG_error_desc_provider = NULL;
    DIAG_OutputDevice = NULL;
#if defined HAL_ENABLE_OS && (HAL_ENABLE_OS > 0)
    OSMUTEX_Destroy(DIAG_OutputDeviceGuard);
    DIAG_OutputDeviceGuard = NULL;
#endif
} /* DIAG_Deinit */

// -----------------------------------------------------------------------------
//  DIAG_SetOutputDevice
// -----------------------------------------------------------------------------
void DIAG_SetOutputDevice(IODevice iodevice) {
    DIAG_OutputDevice = iodevice;
} /* DIAG_SetOutputDevice */

// -----------------------------------------------------------------------------
//  DIAG_GetOutputDevice
// -----------------------------------------------------------------------------
IODevice DIAG_GetOutputDevice(void) {
    return DIAG_OutputDevice;
} /* DIAG_GetOutputDevice */

// -----------------------------------------------------------------------------
//  DIAG_LogChar
//------------------------------------------------------------------------------
void DIAG_LogChar(char character) {
    if (DIAG_OutputDevice) {
        IODEV_Write(DIAG_OutputDevice, (void* )&character, 1, 0);
    }
} /* DIAG_LogChar */

// -----------------------------------------------------------------------------
//  DIAG_LogINT
// -----------------------------------------------------------------------------
void DIAG_LogINT(int32_t value, uint8_t base) {
    int32_t i, size;
    int32_t x, digit; // digit value

    if (DIAG_OutputDevice) {
        //check sign
        if (value < 0) {
            buffLogUINT[0] = '-';
            value = -value;
            size = 2;
        } else {
            size = 1;
        }

        //compute number of digits
        digit = base;
        for (; value >= digit; size++) {
            x = digit * base;
            if (digit > x) {
                //overflow - 0xFFFFFFFF reached
                size++;
                break;
            } else
                digit = x;
        }

        //convert
        x = 1;
        for (i = 1; size - i >= 0; i++) {
            digit = (value / x) % base;
            buffLogUINT[size - i] = '0' + (char) digit + (digit > 9) * ('A' - 10 - '0');
            x *= base;
        }

        IODEV_Write(DIAG_OutputDevice, buffLogUINT, size, 0);
    }
} /* DIAG_LogINT */

// -----------------------------------------------------------------------------
//  DIAG_LogMsg
// -----------------------------------------------------------------------------
void DIAG_LogMsg(const char *msg) {
    if (DIAG_OutputDevice) {
        IODEV_Write(DIAG_OutputDevice, (void* )msg, strlen(msg), 0);
    }
} /* DIAG_LogMsg */

// -----------------------------------------------------------------------------
//  DIAG_LogNL
// -----------------------------------------------------------------------------
void DIAG_LogNL(void) {
    if (DIAG_OutputDevice) {
#if (HAL_DIAG_NL_MODE == 0)
        IODEV_Write(DIAG_OutputDevice, "\n", 1, 0);
#endif
#if (HAL_DIAG_NL_MODE == 1)
        IODEV_Write(DIAG_OutputDevice, "\n\r", 2, 0);
#endif
    }
} /* DIAG_LogNL */

// -----------------------------------------------------------------------------
//  DIAG_LogUINT
// -----------------------------------------------------------------------------
void DIAG_LogUINT(uint32_t value, uint8_t base) {
    int32_t i, size;
    uint32_t x, digit;

    if (DIAG_OutputDevice) {
        //compute number of digits
        digit = base;
        for (size = 1; value >= digit; size++) {
            x = digit * base;
            if (digit > x) {
                //overflow - 0xFFFFFFFF reached
                size++;
                break;
            } else
                digit = x;
        }

        //convert
        x = 1;
        for (i = 1; size - i >= 0; i++) {
            digit = (value / x) % base;
            buffLogUINT[size - i] = '0' + (char) digit + (digit > 9) * ('A' - 10 - '0');
            x *= base;
        }

        buffLogUINT[size] = 0x00;

        IODEV_Write(DIAG_OutputDevice, buffLogUINT, size, 0);
    }
} /* DIAG_LogUINT */

// -----------------------------------------------------------------------------
//  DIAG_ProcError
// -----------------------------------------------------------------------------
static void DIAG_ProcError(ErrorDesc *Error) {
    // process error
    if ((DIAG_error_handler) && (0 == DIAG_error_handler(Error->module_id, Error->error_no, Error->code_line, Error->user_data, Error->description))) {
        return;
    }
    if (DIAG_OutputDevice) {
        DIAG_RELEASE_ASSERT_AND_EXECUTE(0 == IODEV_Lock(DIAG_OutputDevice, HAL_DIAG_LOCK_TIMEOUT)) {

#if(HAL_DIAG_ERROR_SEND_AS_FORMATED_DATA)
            // print error as text
#if(HAL_DIAG_USE_ERROR_DESCRIPTIONS)
            if ((!Error->description) && (DIAG_error_desc_provider)) {
                // there's no error description, but we have description provider!
                Error->description = DIAG_error_desc_provider(Error->module_id, Error->error_no, Error->code_line, Error->user_data, Error->description);
            }
            if (Error->description) {
                DIAG_LogMsg(Error->description);
            }
#endif
            DIAG_LogNL();
            DIAG_LogMsg("ErrorNo: ");
            DIAG_LogUINT(Error->error_no, 10);
            DIAG_LogNL();
            DIAG_LogMsg("Module:  ");
            DIAG_LogUINT(Error->module_id, 10);
            DIAG_LogNL();
            DIAG_LogMsg("Line:    ");
            DIAG_LogUINT(Error->code_line, 10);
#if(HAL_DIAG_USE_ERROR_TIME_STAMPS)
            DIAG_LogNL();
            DIAG_LogMsg("Time:    ");
            DIAG_LogUINT((uint32_t) Error->time, 10);
#endif                   
            DIAG_LogNL();

#else // HAL_DIAG_ERROR_SEND_AS_FORMATED_DATA
            // print error descriptor in binary form
            IODEV_Write(DIAG_OutputDevice, &Error, sizeof(ErrorDesc), 0);
#endif  
            IODEV_Unlock(DIAG_OutputDevice);
        } // if (IODEV_Lock)
    } // if (DIAG_OutputDevice)
} /* DIAG_ProcError */

// -----------------------------------------------------------------------------
//  DIAG_ReportError
// -----------------------------------------------------------------------------
void DIAG_ReportError(uint16_t module_id, uint16_t error_no, uint32_t code_line, void *user_data, char *description) {
#if (HAL_DIAG_USE_ERROR_BUFFERING)
    size_t buf_space;
#endif
    ErrorDesc Error;

#if (HAL_DIAG_USE_ERROR_BUFFERING)
    // buffered mode
    buf_space = IOBUF_GetSpace(ErrorBuf);
    if (buf_space > 1) {
        Error.module_id = module_id;
        Error.error_no = error_no;
        Error.code_line = code_line;
        Error.user_data = user_data;
        Error.description = description;
#if(HAL_DIAG_USE_ERROR_TIME_STAMPS)
        Error.time = CORE_GetSystemTime();
#endif
        IOBUF_Write(ErrorBuf, &Error, 1);
    } else {
        // no free space in the buffer, warn the user
        if (buf_space) {
            Error.module_id = 0;
            Error.error_no = 0;
            Error.code_line = (uint16_t) __LINE__;
            Error.user_data = 0;
#if(HAL_DIAG_USE_ERROR_DESCRIPTIONS)
            Error.description = (char*) ErrBufFullMsg;
#endif
#if(HAL_DIAG_USE_ERROR_TIME_STAMPS)
            Error.time = CORE_GetSystemTime();
#endif
            IOBUF_Write(ErrorBuf, &Error, 1);
        }
    }
#else 
    // non-buffered mode
    Error.module_id = module_id;
    Error.error_no = error_no;
    Error.code_line = code_line;
    Error.user_data = user_data;
    Error.description = description;
#if(HAL_DIAG_USE_ERROR_TIME_STAMPS)
    Error.time = CORE_GetSystemTime();
#endif
    DIAG_ProcError(&Error);
#endif
} /* DIAG_ReportError */

// -----------------------------------------------------------------------------
//  DIAG_ProcessErrors
// -----------------------------------------------------------------------------
uint32_t DIAG_ProcessErrors(uint32_t max_error_count) {
#if (HAL_DIAG_USE_ERROR_BUFFERING)
    size_t b_num_elements;
    ErrorDesc Error;

    b_num_elements = IOBUF_GetCount(ErrorBuf);

    while ((b_num_elements) && (max_error_count)) {
        IOBUF_Read(ErrorBuf, &Error, 1);
        DIAG_ProcError(&Error);
        b_num_elements--;
        max_error_count--;
    }
    return (uint32_t) IOBUF_GetCount(ErrorBuf);

#endif
    return 0;
} /* DIAG_ProcessErrors */

// -----------------------------------------------------------------------------
//  DIAG_SetErrorHandler
// -----------------------------------------------------------------------------
void DIAG_SetErrorHandler(int (*error_handler)(uint16_t, uint16_t, uint16_t, void*, const char*)) {
    DIAG_error_handler = error_handler;
} /* DIAG_SetErrorHandler */

// -----------------------------------------------------------------------------
//  DIAG_SetErrorDescriptionProvider
// -----------------------------------------------------------------------------
void DIAG_SetErrorDescriptionProvider(char* (*error_desc_provider)(uint16_t, uint16_t, uint16_t, void*, const char*)) {
    DIAG_error_desc_provider = error_desc_provider;
} /* DIAG_SetErrorDescriptionProvider */

// -----------------------------------------------------------------------------
//  DIAG_PrintIODeviceInfo
// -----------------------------------------------------------------------------
void DIAG_PrintIODeviceInfo(IODevice iodevice, int indent) {
    int i;
    DIAG_INDENT(i, indent);
    DIAG_LogMsg("io_mode = ");
    switch (iodevice->ioperipheral->iomode) {
        case IODEVICE_MODE_DIRECT:
            DIAG_LogMsg("DIRECT");
            break;
        case IODEVICE_MODE_BUFFERED:
            DIAG_LogMsg("BUFFERED");
            break;
        case IODEVICE_MODE_EVENT:
            DIAG_LogMsg("EVENT");
            break;
        case IODEVICE_MODE_DMA:
            DIAG_LogMsg("DMA");
            break;
        default:
            DIAG_LogMsg("error!");
            return;
            break;
    }
    DIAG_LogMsg(", tx_state = ");
    DIAG_LogINT(iodevice->ioperipheral->tx_state, 10);
    DIAG_LogMsg(", rx_state = ");
    DIAG_LogINT(iodevice->ioperipheral->rx_state, 10);
    DIAG_LogNL();
    DIAG_INDENT(i, indent);
    if (iodevice->ioperipheral->TXBuf) {
        DIAG_LogMsg("TXBuf = 0x");
        DIAG_LogUINT((uint32_t) iodevice->ioperipheral->TXBuf, 16);
        DIAG_LogNL();
        DIAG_PrintIOBufInfo(iodevice->ioperipheral->TXBuf, indent + 2);
    } else {
        DIAG_LogMsg("TXBuf = NULL");
        DIAG_LogNL();
    }
    DIAG_INDENT(i, indent);
    if (iodevice->ioperipheral->RXBuf) {
        DIAG_LogMsg("RXBuf = 0x");
        DIAG_LogUINT((uint32_t) iodevice->ioperipheral->RXBuf, 16);
        DIAG_LogNL();
        DIAG_PrintIOBufInfo(iodevice->ioperipheral->RXBuf, indent + 2);
    } else {
        DIAG_LogMsg("RXBuf = NULL");
        DIAG_LogNL();
    }

} /* DIAG_PrintIODeviceInfo */

// -----------------------------------------------------------------------------
//  DIAG_PrintIOBufInfo
// -----------------------------------------------------------------------------
void DIAG_PrintIOBufInfo(IOBuf iobuf, int indent) {
    int i;
    DIAG_INDENT(i, indent);
    DIAG_LogMsg("elem_size = ");
    DIAG_LogINT(iobuf->elem_size, 10);
    DIAG_LogMsg(", max_elem_count = ");
    DIAG_LogINT(iobuf->max_elem_count, 10);
    DIAG_LogMsg(", elem_count = ");
    DIAG_LogINT(iobuf->elem_count, 10);
    DIAG_LogNL();
    DIAG_INDENT(i, indent);
    DIAG_LogMsg("mem_ptr = 0x");
    DIAG_LogUINT((uint32_t) iobuf->mem_ptr, 16);
    DIAG_LogMsg(", read_ptr = 0x");
    DIAG_LogUINT((uint32_t) iobuf->read_ptr, 16);
    DIAG_LogMsg(", write_ptr = 0x");
    DIAG_LogUINT((uint32_t) iobuf->write_ptr, 16);
    DIAG_LogNL();

} /* DIAG_PrintIOBufInfo */
