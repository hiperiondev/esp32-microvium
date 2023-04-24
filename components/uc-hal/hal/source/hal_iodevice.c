/*
 * @file hal_iodevice.c
 * @brief HAL IODevice interface
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include "hal_iodevice.h"
#if (HAL_IO_OS_INTEGRATION)
#include "hal_core.h"
#include "hal_os.h"
#endif

typedef size_t (*TGETFUNC)(IOPeripheral ioperipheral);
typedef size_t (*TIORDFUNC)(IOPeripheral ioperipheral, void *data, size_t size, unsigned int timeout);
typedef size_t (*TIOWRFUNC)(IOPeripheral ioperipheral, const void *data, size_t size, unsigned int timeout);

size_t IODEV_GetReadCountBuf(IOPeripheral ioperipheral);
size_t IODEV_GetWriteSpaceBuf(IOPeripheral ioperipheral);
size_t IODEV_ReadPeripheralBuf(IOPeripheral ioperipheral, void *data, size_t size, unsigned int timeout);
size_t IODEV_WritePeripheralBuf(IOPeripheral ioperipheral, const void *data, size_t size, unsigned int timeout);
static size_t IODEV_DummyGet(IOPeripheral ioperipheral);
static size_t IODEV_DummyRead(IOPeripheral ioperipheral, void *data, size_t size, unsigned int timeout);
static size_t IODEV_DummyWrite(IOPeripheral ioperipheral, const void *data, size_t size, unsigned int timeout);

// -----------------------------------------------------------------------------
//  IODEV_Init
// -----------------------------------------------------------------------------
HALRESULT IODEV_Init(IODevice iodevice, void *init_data) {
    HALRESULT result = HALRESULT_ERROR;
    if (iodevice) {
        if (iodevice->ioperipheral) {
            if (iodevice->ioperipheral->Init) {
                result = iodevice->ioperipheral->Init(iodevice->ioperipheral, init_data);
            }
#if (HAL_IO_OS_INTEGRATION)
            // in OS INTEGRATION MODE, create synchronization semaphore for data reception
            iodevice->ioperipheral->rx_notification_threshold = 0;
            iodevice->ioperipheral->rx_notification_sem = OSSEM_Create();
            if (iodevice->ioperipheral->rx_notification_sem == NULL) {
                IODEV_Deinit(iodevice);
                return HALRESULT_OSSEM_CREATION_FAILED;
            }
#endif
        }
    }
    return result;
} /* IODEV_Init */

// -----------------------------------------------------------------------------
//  IODEV_Deinit
// -----------------------------------------------------------------------------
HALRESULT IODEV_Deinit(IODevice iodevice) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            if (iodevice->ioperipheral->DeInit) {
                iodevice->ioperipheral->DeInit(iodevice->ioperipheral);
            }
#if (HAL_IO_OS_INTEGRATION)
            // in OS INTEGRATION MODE, destroy synchronization semaphore for data reception
            if (iodevice->ioperipheral->rx_notification_sem != NULL) {
                OSSEM_Destroy(iodevice->ioperipheral->rx_notification_sem);
                iodevice->ioperipheral->rx_notification_sem = NULL;
            }
#endif
        }
    }
    return HALRESULT_OK;
} /* IODEV_Deinit */

// -----------------------------------------------------------------------------
//  IODEV_DisableRead
// -----------------------------------------------------------------------------
void IODEV_DisableRead(IODevice iodevice) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->rx_state = 0;
            if (iodevice->ioperipheral->DisableRead) {
                iodevice->ioperipheral->DisableRead(iodevice->ioperipheral);
            }
        }
    }

} /* IODEV_DisableRead */

// -----------------------------------------------------------------------------
//  IODEV_DisableWrite
// -----------------------------------------------------------------------------
void IODEV_DisableWrite(IODevice iodevice) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->tx_state = 0;
            if (iodevice->ioperipheral->DisableWrite) {
                iodevice->ioperipheral->DisableWrite(iodevice->ioperipheral);
            }
        }
    }
} /* IODEV_DisableWrite */

// -----------------------------------------------------------------------------
//  IODEV_EnableRead
// -----------------------------------------------------------------------------
void IODEV_EnableRead(IODevice iodevice) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->rx_state = 1;
            if (iodevice->ioperipheral->EnableRead) {
                iodevice->ioperipheral->EnableRead(iodevice->ioperipheral);
            }
        }
    }
} /* IODEV_EnableRead */

// -----------------------------------------------------------------------------
//  IODEV_EnableWrite
// -----------------------------------------------------------------------------
void IODEV_EnableWrite(IODevice iodevice) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->tx_state = 1;
            if (iodevice->ioperipheral->EnableWrite) {
                iodevice->ioperipheral->EnableWrite(iodevice->ioperipheral);
            }
        }
    }
} /* IODEV_EnableWrite */

// -----------------------------------------------------------------------------
//  IODEV_GetReadCountBuf
// -----------------------------------------------------------------------------
size_t IODEV_GetReadCountBuf(IOPeripheral ioperipheral) {
    return (IOBUF_GetCount(ioperipheral->RXBuf));
} /* IODEV_GetReadCountBuf */

// -----------------------------------------------------------------------------
//  IODEV_GetWriteSpaceBuf
// -----------------------------------------------------------------------------
size_t IODEV_GetWriteSpaceBuf(IOPeripheral ioperipheral) {
    return (IOBUF_GetSpace(ioperipheral->TXBuf));
} /* IODEV_GetWriteSpaceBuf */

// -----------------------------------------------------------------------------
//  IODEV_ReadPeripheralBuf
// -----------------------------------------------------------------------------
size_t IODEV_ReadPeripheralBuf(IOPeripheral ioperipheral, void *data, size_t size, unsigned int timeout) {
    size_t rxsize = 0;

#if (HAL_IO_OS_INTEGRATION)
    if (timeout) {
        // wait until data is ready
        IOPERIPH_WaitForData(ioperipheral, size, timeout);
    }
#endif
    // read from the receive buffer
    rxsize = IOBUF_Read(ioperipheral->RXBuf, data, size);

    return rxsize;

} /* IODEV_ReadPeripheralBuf */

// -----------------------------------------------------------------------------
//  IODEV_SetMode
// -----------------------------------------------------------------------------
void IODEV_SetMode(IODevice iodevice, IODeviceMode iomode) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            if (iodevice->ioperipheral->SetMode) {
                iomode = iodevice->ioperipheral->SetMode(iodevice->ioperipheral, iomode);
            }
            switch (iomode) {
                case IODEVICE_MODE_DIRECT:
                    // in DIRECT mode of operation, the read and write functions map directly to the ioperipheral's implementations
                    iodevice->GetReadCount = (TGETFUNC) (iodevice->ioperipheral->GetReadCount);
                    iodevice->GetWriteSpace = (TGETFUNC) (iodevice->ioperipheral->GetWriteSpace);
                    iodevice->WritePeripheral = (TIOWRFUNC) iodevice->ioperipheral->WriteDirect;
                    iodevice->ReadPeripheral = (TIORDFUNC) iodevice->ioperipheral->ReadDirect;
                    break;
                case IODEVICE_MODE_BUFFERED:
                    // in BUFFERED mode of operation...
                    iodevice->GetReadCount = IODEV_GetReadCountBuf;
                    iodevice->GetWriteSpace = IODEV_GetWriteSpaceBuf;
                    iodevice->WritePeripheral = IODEV_WritePeripheralBuf;
                    iodevice->ReadPeripheral = IODEV_ReadPeripheralBuf;
                    break;
                case IODEVICE_MODE_EVENT:
                    // in EVENT mode of operation...
                    iodevice->GetReadCount = IODEV_DummyGet;
                    iodevice->GetWriteSpace = IODEV_DummyGet;
                    iodevice->WritePeripheral = IODEV_DummyWrite;
                    iodevice->ReadPeripheral = IODEV_DummyRead;
                    break;
                case IODEVICE_MODE_DMA:
                    // in DMA mode of operation...
                    iodevice->GetReadCount = IODEV_GetReadCountBuf;
                    iodevice->GetWriteSpace = IODEV_GetWriteSpaceBuf;
                    iodevice->WritePeripheral = (TIOWRFUNC) iodevice->ioperipheral->WriteDMA;
                    iodevice->ReadPeripheral = (TIORDFUNC) iodevice->ioperipheral->ReadDMA;
                    break;
                default:
                    break;
            } // switch (iomode)

            // save mode
            iodevice->ioperipheral->iomode = iomode;

        } // if (iodevice->ioperipheral)
    } // if (iodevice)
} /* IODEV_SetMode */

// -----------------------------------------------------------------------------
//  IODEV_SetPeripheral
// -----------------------------------------------------------------------------
void IODEV_SetPeripheral(IODevice iodevice, IOPeripheral ioperipheral) {
    if (iodevice) {
        iodevice->ioperipheral = ioperipheral;
    }
} /* IODEV_SetPeripheral */

// -----------------------------------------------------------------------------
//  IODEV_SetRXBuffer
// -----------------------------------------------------------------------------
void IODEV_SetRXBuffer(IODevice iodevice, IOBuf iobuf) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->RXBuf = iobuf;
        }
    }
} /* IODEV_SetRXBuffer */

// -----------------------------------------------------------------------------
//  IODEV_SetTXBuffer
// -----------------------------------------------------------------------------
void IODEV_SetTXBuffer(IODevice iodevice, IOBuf iobuf) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->TXBuf = iobuf;
        }
    }
} /* IODEV_SetTXBuffer */

// -----------------------------------------------------------------------------
//  IODEV_SetReceiveEventHandler
// -----------------------------------------------------------------------------
void IODEV_SetReceiveEventHandler(IODevice iodevice, EVENT_HANDLER_FUNC rx_handler_func) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->ReceiveEventHandler = rx_handler_func;
        }
    }
} /* IODEV_SetReceiveEventHandler */

// -----------------------------------------------------------------------------
//  IODEV_SetReceiveErrorHandler
// -----------------------------------------------------------------------------
void IODEV_SetReceiveErrorHandler(IODevice iodevice, ERROR_HANDLER_FUNC rx_error_handler_func) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->ReceiveErrorHandler = rx_error_handler_func;
        }
    }
} /* IODEV_SetReceiveErrorHandler */

// -----------------------------------------------------------------------------
//  IODEV_SetTransmitEventHandler
// -----------------------------------------------------------------------------
void IODEV_SetTransmitEventHandler(IODevice iodevice, EVENT_HANDLER_FUNC tx_handler_func) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->TransmitEventHandler = tx_handler_func;
        }
    }
} /* IODEV_SetTransmitEventHandler */

// -----------------------------------------------------------------------------
//  IODEV_SetTransmitErrorHandler
// -----------------------------------------------------------------------------
void IODEV_SetTransmitErrorHandler(IODevice iodevice, ERROR_HANDLER_FUNC tx_error_handler_func) {
    if (iodevice) {
        if (iodevice->ioperipheral) {
            iodevice->ioperipheral->TransmitErrorHandler = tx_error_handler_func;
        }
    }
} /* IODEV_SetTransmitErrorHandler */

// -----------------------------------------------------------------------------
//  IODEV_Lock
// -----------------------------------------------------------------------------
int IODEV_Lock(IODevice iodevice, unsigned int timeout) {
#if defined HAL_IO_OS_INTEGRATION && (HAL_IO_OS_INTEGRATION != 0)
    if (iodevice) {
        if (iodevice->ioperipheral) {
            if (NULL == iodevice->ioperipheral->lock) {
                // first call to IODEV_Lock must create the semaphore
                iodevice->ioperipheral->lock = OSMUTEX_Create();
            }
            if (iodevice->ioperipheral->lock) {
                return OSMUTEX_Take(iodevice->ioperipheral->lock, timeout);
            }
        }
    }
    return 1;

#endif

    return 0;

} /* IODEV_Lock */

// -----------------------------------------------------------------------------
//  IODEV_Unlock
// -----------------------------------------------------------------------------
void IODEV_Unlock(IODevice iodevice) {
#if defined HAL_IO_OS_INTEGRATION && (HAL_IO_OS_INTEGRATION != 0)
    if (iodevice) {
        if (iodevice->ioperipheral) {
            if (iodevice->ioperipheral->lock) {
                OSMUTEX_Give(iodevice->ioperipheral->lock);
            }
        }
    }
#endif

} /* IODEV_Unlock */

// -----------------------------------------------------------------------------
//  IODEV_WritePeripheralBuf
// -----------------------------------------------------------------------------
size_t IODEV_WritePeripheralBuf(IOPeripheral ioperipheral, const void *data, size_t size, unsigned int timeout) {
    size_t txsize = 0;
    txsize = IOBUF_Write(ioperipheral->TXBuf, data, size);
    if (ioperipheral->tx_state) {
        if (ioperipheral->EnableWrite) {
            ioperipheral->EnableWrite(ioperipheral);
        }
    }
    return txsize;

} /* IODEV_WritePeripheralBuf */

// -----------------------------------------------------------------------------
//  IODEV_DummyGet
// -----------------------------------------------------------------------------
static size_t IODEV_DummyGet(IOPeripheral ioperipheral) {
    return 0;
} /* IODEV_DummyGet */

// -----------------------------------------------------------------------------
//  IODEV_DummyRead
// -----------------------------------------------------------------------------
static size_t IODEV_DummyRead(IOPeripheral ioperipheral, void *data, size_t size, unsigned int timeout) {
    return 0;
} /* IODEV_DummyReadWrite */

// -----------------------------------------------------------------------------
//  IODEV_DummyWrite
// -----------------------------------------------------------------------------
static size_t IODEV_DummyWrite(IOPeripheral ioperipheral, const void *data, size_t size, unsigned int timeout) {
    return 0;
} /* IODEV_DummyReadWrite */
