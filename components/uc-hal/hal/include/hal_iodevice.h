/*
 * @file hal_iodevice.h
 * @brief HAL IODevice API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_IODEVICE_H
#define HAL_IODEVICE_H

#include "hal_ioperiph.h"

/** \defgroup hal_iodevice IODevice interface
 * 
 * The most important IO module interface is the <b>IODevice</b> interface. The application 
 * interacts with the IODevice objects. Each peripheral entity in use has it's own 
 * IODevice object instance but any of these objects provide a common, unified 
 * interface for sending and receiving data. For example, if there are two UARTs 
 * used on board of the microcontroller, there will be two IODevice instances present,
 * but they will all share a common API. The IODevice object basically acts as a data 
 * endpoint and bridge between peripherals and application, but the application may
 * not care what type of underlying peripheral is in use, as long as it's sends/receives
 * the desired data. The IODevice functionality includes buffering of data, signaling
 * transaction events including transfer errors, utilizing DMA transfers. The IODevice API 
 * provides several functions, below is the complete list with short descriptions. 
 * <center>
 * <table>
 * <tr><td colspan = "2"> <center><b>Initialization</b></center> </td></tr>
 * <tr><td>\ref IODEV_Init          </td><td>  Initializes the IODevice</td></tr>
 * <tr><td>\ref IODEV_Deinit        </td><td>  Deinitializes the IODevice</td></tr>
 * 
 * <tr><td colspan = "2"> <center><b>Controlling mode of operation</b></center> </td></tr>
 * <tr><td>\ref IODEV_SetMode       </td><td>  Sets desired mode of operation for the specified IODevice</td></tr>
 * <tr><td>\ref IODEV_GetMode       </td><td>  Gets the currently set mode of operation for the specified IODevice</td></tr>
 * 
 * <tr><td colspan = "2"> <center><b>Controlling flow of data</b></center> </td></tr>
 * <tr><td>\ref IODEV_EnableRead    </td><td>  Enables reception for the specified IODevice</td></tr>
 * <tr><td>\ref IODEV_EnableWrite   </td><td>  Enables transmission for the specified IODevice</td></tr>
 * <tr><td>\ref IODEV_DisableRead   </td><td>  Disables reception for the specified IODevice</td></tr>
 * <tr><td>\ref IODEV_DisableWrite  </td><td>  Disables transmission for the specified IODevice</td></tr>
 * 
 * <tr><td colspan = "2"> <center><b>Controlling state of transfer</b></center> </td></tr>
 * <tr><td>\ref IODEV_GetReadCount  </td><td>  Gets the amount of data available for reading in the IODevice</td></tr>
 * <tr><td>\ref IODEV_GetWriteSpace </td><td>  Gets the amount of free buffering space in the IODevice</td></tr>
 * 
 * <tr><td colspan = "2"> <center><b>Reading and writing</b></center> </td></tr>
 * <tr><td>\ref IODEV_Read          </td><td>  Reads data from IODevice</td></tr>
 * <tr><td>\ref IODEV_Write         </td><td>  Writes data to IODevice</td></tr>
 * 
 * <tr><td colspan = "2"> <center><b>Object binding</b></center> </td></tr>
 * <tr><td>\ref IODEV_SetPeripheral </td><td>  Binds a peripheral with IODevice</td></tr>
 * <tr><td>\ref IODEV_SetRXBuffer   </td><td>  Associates reception buffer with IODevice</td></tr>
 * <tr><td>\ref IODEV_SetTXBuffer   </td><td>  Associates transmission buffer with IODevice</td></tr>

 * <tr><td colspan = "2"> <center><b>Event handling</b></center> </td></tr>
 * <tr><td>\ref IODEV_SetReceiveEventHandler    </td><td>  Sets a handler for reception events</td></tr>
 * <tr><td>\ref IODEV_SetReceiveErrorHandler    </td><td>  Sets a handler for reception errors</td></tr>
 * <tr><td>\ref IODEV_SetTransmitEventHandler   </td><td>  Sets a handler for transmission events</td></tr>
 * <tr><td>\ref IODEV_SetTransmitErrorHandler   </td><td>  Sets a handler for transmission errors</td></tr>
 * </table> 
 * </center>
 *
 *  
 * <b>IODevice modes of operation.</b>
 *
 * The IODevice interface supports 4 modes of operation. These modes control the way the
 * IODevice objects behave and what resources they use to handle communication. The mode 
 * of operation does NOT control the way the data is formatted and for example sent through 
 * the underlying physical interface. It just controls how data is handled inside the 
 * microcontroller. The modes are listed below, along with their detailed description:
 * 
 * - <b>DIRECT</b> mode is the simplest and most straightforward mode of communication. 
 *   It does not use interrupts nor DMA channels. The only buffering capabilities are
 *   provided by the port implementation and are considered to be device dependent.
 *   In most implementations, the IODevice API would interact directly with low level 
 *   microcontroler peripheral registers, writing data to them and reading from them.
 * 
 * - <b>BUFFERED</b> mode uses interrupts and data buffering. When writing to IODevice,
 *   the data is stored in an internal IOBuffer (the so called TXBuffer) and fed to the 
 *   peripheral during the transmit interrupt service routine. When reading, the data 
 *   is read from the internal IOBuffer (so called RXBuffer), which is filled with data
 *   during the receive interrupt service routine.
 * 
 * - <b>EVENT-BASED</b> mode uses interrupts, but no internal data buffering. The user
 *   is responsible for providing special functions, that are called in an interrupt
 *   service routine that act as data sink (for reading) and data source (for writing).
 *   
 * - <b>DMA-BASED</b> mode uses interrupts and DMA channels. This mode is available only
 *   on architectures that support DMA. Other architectures would use the BUFFERED mode,
 *   as it's the closest matching mode. 
 *
 * The mode of operation is selected by calling \ref IODEV_SetMode. Passing <i>IODEVICE_MODE_DEFAULT</i>
 * as a parameter will allow the port to select the default mode. Application may check 
 * what mode the IODevice is currently in by calling \ref IODEV_GetMode.
 *  
 * <b>Controlling the flow of data.</b>
 * 
 * Application can enable or disable the receive or transmit path for the IODevice. Disabling
 * the transmit path by calling \ref IODEV_DisableWrite results in no output from the
 * micronontroller. That means there is no data flow. However, it does not mean that 
 * application cannot write to the IODevice. The application can write to it as long as
 * there is enough buffering space. Application may check that space it by calling 
 * \ref IODEV_GetWriteSpace. This is particularly useful in BUFFERED and DMA modes of
 * operation, where data can be prepared before it is actually sent, especially if the 
 * preparing time is quite long.
 * When disabling the receive path by calling \ref IODEV_DisableRead, the external data 
 * will not be received any longer. However, the data that came before the disable call,
 * but have not been processed can still be read from the IODevice. The IODEV_GetReadCount
 * always shows how much data is left in the IODevice to read.  
 * 
 * <b>IO events.</b>
 * 
 * The application may be notified about several transfer events. This feature is useful
 * for synchronizing application tasks.
 */
/*@{*/

// -----------------------------------------------------------------------------
//  PUBLIC TYPEDEFS
// -----------------------------------------------------------------------------
/** Descriptor of an IODevice */
typedef struct {
    /**
     * handle of the ioperipheral associated with the iodevice
     */
    IOPeripheral ioperipheral;
    /** 
     *  Depending on iodevice configuration, this function pointer can point to either
     *  GetReadCount function of the associated ioperipheral (direct mode access) or
     *  GetReadCountBuf function (buffered mode access)
     *  
     *  @param ioperipheral
     */
    size_t (*GetReadCount)(IOPeripheral ioperipheral);

    /** 
     *  Depending on iodevice configuration, this function pointer can point to either
     *  GetWriteSpace function of the associated ioperipheral (direct mode access) or
     *  GetWriteSpaceBuf function (buffered mode access)
     *  
     *  @param ioperipheral
     */
    size_t (*GetWriteSpace)(IOPeripheral ioperipheral);

    /** 
     *  Depending on iodevice configuration, this function pointer can point to either
     *  ReadDirect function of the associated ioperipheral (direct mode access) or
     *  ReadPeripheralBuf function (buffered mode access)
     *  
     *  @param ioperipheral
     *  @param data
     *  @param size
     *  @param timeout timeout value (in miliseconds). This parameter is ignored and
     *  should be set to 0 when OS integration is disabled. 
     */
    size_t (*ReadPeripheral)(IOPeripheral ioperipheral, void *data, size_t size, unsigned int timeout);

    /** 
     *  Depending on iodevice configuration, this function pointer can point to either
     *  WriteDirect function of the associated ioperipheral (direct mode access) or
     *  WritePeripheralBuf function (buffered mode access)
     *  
     *  @param ioperipheral
     *  @param data
     *  @param size
     *  @param timeout timeout value (in miliseconds). This parameter is ignored and
     *  should be set to 0 when OS integration is disabled. 
     */
    size_t (*WritePeripheral)(IOPeripheral ioperipheral, const void *data, size_t size, unsigned int timeout);

} IODeviceDesc, *IODevice;

// -----------------------------------------------------------------------------
//  PUBLIC API
// -----------------------------------------------------------------------------

/** 
 *  Returns the number of bytes available for reading from the device. Implemented
 *  as a macro that calls iodevice->GetReadCount with iodevice->ioperipheral as a
 *  parameter
 *  
 *  @param iodevice handle of the iodevice
 */
#define IODEV_GetReadCount(iodevice)                    (iodevice)->GetReadCount((iodevice)->ioperipheral) 

/** 
 *  Returns the number of bytes that can be written to the device.
 *  
 *  @param iodevice handle of the iodevice
 */
#define IODEV_GetWriteSpace(iodevice)                   (iodevice)->GetWriteSpace((iodevice)->ioperipheral)

/** 
 *  Reads a specified number of elements from the iodevice.
 *  
 *  @param iodevice handle of the iodevice
 *  @param data pointer to the location where data will be stored
 *  @param size size of data to read
 *  @param timeout timeout value (in miliseconds). This parameter is ignored and
 *  should be set to 0 when OS integration is disabled. 
 */
#define IODEV_Read(iodevice, data, size, timeout)       (iodevice)->ReadPeripheral((iodevice)->ioperipheral, data, size, timeout)

/** 
 *  Writes a specified number of elements to the iodevice.
 *  
 *  @param iodevice handle of the iodevice
 *  @param data pointer to the data to write
 *  @param size size of the data to write
 *  @param timeout timeout value (in miliseconds). This parameter is ignored and
 *  should be set to 0 when OS integration is disabled. 
 */
#define IODEV_Write(iodevice, data, size, timeout)      (iodevice)->WritePeripheral((iodevice)->ioperipheral, data, size, timeout)

/**
 * Retrieves the current mode of operation for specified iodevice
 * @param iodevice handle of the iodevice
 * @return mode of operation
 */
#define IODEV_GetMode(iodevice)                         ((iodevice)->ioperipheral->iomode)

/**
 * Blocks the calling task until a specified number of elements is received by IODevice
 * or timeout occurs. Works in IO_INTEGRATION_MODE only!
 * @param iodevice handle of the iodevice
 * @param size number of elements to wait for
 * @param timeout timeout value (in miliseconds)
 * @return 1 if the specified amount of data was received in the specified time
 *         0 if timeout occurred before the specified amount of data was received
 */
#define IODEV_WaitForData(iodevice, size, timeout)      IOPERIPH_WaitForData((iodevice)->ioperipheral, (size), (timeout))

/** 
 *  Initializes the peripheral, associated with the iodevice using data specified
 *  by the init_data parameter. The initialization data format is different, for
 *  different peripherals. If this parameter is NULL, the peripheral is initialized
 *  with default settings (configured at compile time)
 *  
 *  @param iodevice handle of the iodevice
 *  @param init_data pointer to the additional initialization data, that will be
 *  passed to the ioperipheral object during initialization (can be ommited by
 *  passing NULL) 
 */
HALRESULT IODEV_Init(IODevice iodevice, void *init_data);

/** 
 *  Deinitializes the peripheral, associated with the iodevice.
 *  
 *  @param iodevice handle of the iodevice
 */
HALRESULT IODEV_Deinit(IODevice iodevice);

/** 
 *  Enables the receive path.
 *  
 *  @param iodevice handle of the iodevice
 */
void IODEV_EnableRead(IODevice iodevice);

/** 
 *  Disables the receive path.
 *  
 *  @param iodevice handle of the iodevice
 */
void IODEV_EnableWrite(IODevice iodevice);

/** 
 *  Disables the receive path.
 *  
 *  @param iodevice handle of the iodevice
 */
void IODEV_DisableRead(IODevice iodevice);

/** 
 *  Disables the transmit path.
 *  
 *  @param iodevice handle of the iodevice
 */
void IODEV_DisableWrite(IODevice iodevice);

/** 
 *  Sets a mode of operation for a specified iodevice. Available modes are: DIRECT
 *  (default mode), BUFFERED, EVENT-BASED and DMA-BASED (on architectures
 *  supporting DMA).
 *  
 *  @param iodevice handle of the iodevice
 *  @param iomode io mode of operation
 */
void IODEV_SetMode(IODevice iodevice, IODeviceMode iomode);

/** 
 *  Assigns a peripheral device to a specified io device.
 *  
 *  @param iodevice handle of the iodevice
 *  @param ioperipheral handle of the ioperipheral to associate with iodevice
 */
void IODEV_SetPeripheral(IODevice iodevice, IOPeripheral ioperipheral);

/** 
 *  Assigns a buffer as an receive buffer for a specified iodevice.
 *  
 *  @param iodevice handle of the iodevice
 *  @param iobuf iobuf object that will be associated as a receive buffer
 */
void IODEV_SetRXBuffer(IODevice iodevice, IOBuf iobuf);

/** 
 *  Assigns a buffer as an transmit buffer for a specified iodevice.
 *  
 *  @param iodevice handle of the iodevice
 *  @param iobuf iobuf object that will be associated as a transmit buffer
 */
void IODEV_SetTXBuffer(IODevice iodevice, IOBuf iobuf);

/** 
 *  Assigns a receive-error handler function to a specified iodevice.
 *  
 *  @param iodevice handle of the iodevice
 *  @param rx_error_handler_func pointer to the transmit-event handler function
 */
void IODEV_SetReceiveErrorHandler(IODevice iodevice, ERROR_HANDLER_FUNC rx_error_handler_func);

/** 
 *  Assigns a receive-event handler function to a specified iodevice. This function
 *  will be called in a receive ISR (only in EVENT-BASED mode of operation) as a
 *  data sink, that processes incomming data.
 *  
 *  @param iodevice handle of the iodevice
 *  @param rx_handler_func pointer to the receive-event handler function
 */
void IODEV_SetReceiveEventHandler(IODevice iodevice, EVENT_HANDLER_FUNC rx_handler_func);

/** 
 *  Assigns a transmit-error handler function to a specified iodevice.
 *  
 *  @param iodevice handle of the iodevice
 *  @param tx_error_handler_func pointer to the transmit-event handler function
 */
void IODEV_SetTransmitErrorHandler(IODevice iodevice, ERROR_HANDLER_FUNC tx_error_handler_func);

/** 
 *  Assigns a transmit-event handler function to a specified iodevice. This
 *  function will be called in a transmit ISR (only in EVENT-BASED mode of
 *  operation) as a data source, that generates outgoing data.
 *  
 *  @param iodevice handle of the iodevice
 *  @param tx_handler_func pointer to the transmit-event handler function
 */
void IODEV_SetTransmitEventHandler(IODevice iodevice, EVENT_HANDLER_FUNC tx_handler_func);

/**
 *  Locks access to a specified IODevice prohibiting concurrent access. This function
 *  is used to guard access to shared IODevice. After acquiring lock it must be released
 *  through a call to \ref IDEV_Unlock.
 *  This function requires HAL_IO_OS_INTEGRATION to be set to non-zero value in hal_config.h
 *
 *  @param iodevice handle of the iodevice
 *  @param timeout maximum time waiting for device
 *
 */
int IODEV_Lock(IODevice iodevice, unsigned int timeout);

/**
 *  Unlocks access to a specified IODevice re-enabling access to it.
 *  This function requires HAL_IO_OS_INTEGRATION to be set to non-zero value in hal_config.h
 *
 *  @param iodevice handle of the iodevice
 *
 */
void IODEV_Unlock(IODevice iodevice);

/*@}*/

#endif /* HAL_IODEVICE_H */

