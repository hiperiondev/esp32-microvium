/*
 * @file hal_ioperiph.h
 * @brief HAL IOPeripheral interface specification
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#ifndef HAL_IOPERIPHERAL_H
#define HAL_IOPERIPHERAL_H

#include <signal.h>

#include "hal_defs.h"
#include "hal_iobuf.h"

#if defined HAL_IO_OS_INTEGRATION && (HAL_IO_OS_INTEGRATION != 0)
    #if defined HAL_ENABLE_OS && (HAL_ENABLE_OS != 0)
        #include "hal_osnotifier.h"
    #else
        #error "HAL_IO_OS_INTEGRATION enabled but HAL_ENABLE_OS not!"
        #error "Check your hal_config.h"
    #endif
#endif

/** \defgroup hal_ioperipheral IOPeripheral interface
 * 
 * The <b>IOPeripheral</b> interface describes the actual peripheral. It is the "bottom" 
 * interface of IODevice, meaning, that any IO device that complies with IOPeripheral
 * interface can be assigned to an IODevice object and be presented to the application
 * as an IODevice object. This interface defines the functionality of a port for HAL IO module.
 * All functions are grouped in a \ref IOPeripheralDesc structure. See documentation of
 * that structure for details on particular functions and their responsibilities.
 */
/*@{*/

// -----------------------------------------------------------------------------
//  PUBLIC TYPEDEFS
// -----------------------------------------------------------------------------
/**
 * Possible IODevice modes of operation
 */
typedef enum {
    /**
     * DEFAULT mode of operation identifier
     */
    IODEVICE_MODE_DEFAULT = 0,
    /**
     * DIRECT mode of operation identifier
     */
    IODEVICE_MODE_DIRECT,
    /**
     * BUFFERED mode of operation identifier
     */
    IODEVICE_MODE_BUFFERED,
    /**
     * DMA-based mode of operation identifier
     */
    IODEVICE_MODE_DMA,
    /**
     * EVENT-based mode of operation identifier
     */
    IODEVICE_MODE_EVENT
} IODeviceMode;

/**
 * Possible IODevice events
 */
typedef enum {
    IOEVENT_SINGLE_ELEMENT_RECEPTION,
    IOEVENT_SINGLE_ELEMENT_TRANSMISSION,
    IOEVENT_END_OF_TRANSMISSION,
    IOEVENT_END_OF_RECEPTION,
    IOEVENT_DATA_UNDERRUN,
    IOEVENT_DATA_OVERFLOW
} IOEventType;

/**
 * A type definition of a function handling IO events
 * @param ioperipheral handle of the ioperipheral
 * @param data data read during the receive event or pointer to the memory area,
 *             where the provided data will be stored during the transmit event
 * @param size number of elements read during the receive event or requested number
 *             of elements, that this event handler should provide during the transmit
 *             event
 * @return
 */
typedef size_t (*EVENT_HANDLER_FUNC)(void *ioperipheral, IOEventType event_type, void *data, size_t size);

/**
 * A type definition of a function handling IO errors 
 * @param ioperipheral handle of the ioperipheral
 * @param error error code
 */
typedef void (*ERROR_HANDLER_FUNC)(void *ioperipheral, uint32_t error);

/** Descriptor of an IOPeripheral */
typedef struct IOPeripheralDesc {
    /**
     * current mode of operation
     */
    IODeviceMode iomode;

    /**
     * Holds the state of transmit path (1 = enabled, 0 = disabled). By default the transmit path
     * is disabled and should be enabled by a call to EnableWrite. This variable can be used by
     * the port, but MUST NOT be modified.
     */
    sig_atomic_t tx_state;

    /**
     * Holds the state of receive path (1 = enabled, 0 = disabled). By default the receive path
     * is disabled and should be enabled by a call to EnableRead. This variable can be used by
     * the port, but MUST NOT be modified.
     */
    sig_atomic_t rx_state;

    /**
     * Receive buffer associated with an ioperipheral
     */
    IOBuf RXBuf;

    /**
     * Transmit buffer associated with an ioperipheral
     */
    IOBuf TXBuf;

    /** 
     *  This function deinitializes the peripheral to it's default (reset) state.
     *  
     *  @param ioperipheral handle of the ioperipheral
     */
    HALRESULT (*DeInit)(void *ioperipheral);

    /** 
     *  This function disables the (previously enabled) receive path (See description
     *  of EnableRead function).
     *  
     *  @param ioperipheral handle of the ioperipheral
     */
    void (*DisableRead)(void *ioperipheral);

    /** 
     *  This function disables the (previously enabled) transmit path (See description
     *  of EnableWrite function).
     *  
     *  @param ioperipheral handle of the ioperipheral
     */
    void (*DisableWrite)(void *ioperipheral);

    /** 
     *  This function enables the receive path of the ioperipheral. It handles both:
     *  peripheral receiver configuration and port (pin) configuration. After calling
     *  this function, the peripheral must be ready to accept new data. This function
     *  is used in all operating modes. In DIRECT mode of operation this usually means
     *  just port (pin) configuration and receiver setup. In the BUFFERED and EVENT
     *  mode proper receive interrupt configuration is also required. In DMA based mode,
     *  proper DMA channel must be also configured.
     *  
     *  @param ioperipheral handle of the ioperipheral
     */
    void (*EnableRead)(void *ioperipheral);

    /** 
     *  This function enables the transmit path of the ioperipheral. It handles both:
     *  peripheral transmitter configuration and port (pin) configuration. After
     *  calling this function, the peripheral must be ready to accept new data to send.
     *  This function is used in all operating modes. In DIRECT mode of operation this
     *  usually means just port (pin) configuration and transmitter setup. In the
     *  BUFFERED and EVENT mode proper transmit interrupt configuration is also
     *  required. In DMA based mode, proper DMA channel must be also configured.
     *  
     *  @param ioperipheral handle of the ioperipheral
     */
    void (*EnableWrite)(void *ioperipheral);

    /** 
     *  This function retrieves the number of elements that the receiver holds in its
     *  internal receive buffer. This function is used only in DIRECT mode of operation.
     *  The buffering capabilities (buffer depth) depend on the port peripheral
     *  capabilities. For ports with no buffering capabilities, this function usually
     *  returns 1 when there is a single element to read in the receive register or 0
     *  when not.
     *  @return  number of elements that the receiver holds in its internal receive
     *  buffer or 0 if there was an error
     *  
     *  @param ioperipheral handle of the ioperipheral
     */
    size_t (*GetReadCount)(void *ioperipheral);

    /** 
     *  This function retrieves the number of elements that the transmitter can still
     *  accept (the number of elements that can still fit into the internal transmit
     *  buffer). This function is used only in DIRECT mode of operation. The buffering
     *  capabilities (buffer depth) depend on the port peripheral capabilities. For
     *  ports with no buffering capabilities, this function usually returns 1 when the
     *  transmitter is in IDLE state or 0 when it's busy.
     *  @return number of elements that the transmitter can still accept or 0 if there
     *  was an error
     *  
     *  @param ioperipheral handle of the ioperipheral
     */
    size_t (*GetWriteSpace)(void *ioperipheral);

    /** 
     *  This function initializes the peripheral port with the port-specyfic data
     *  provided by the init_data parameter. If this parameter is NULL, the peripheral
     *  port is configured with default settings (provided usually at compile time).
     *  
     *  @param ioperipheral handle of the ioperipheral
     *  @param init_data pointer to the initialization data (port-specific) or NULL if
     *  the peripheral is being configured with default settings 
     */
    HALRESULT (*Init)(void *ioperipheral, void *init_data);

    /** 
     *  This function reads a specified number of elements directly from the peripheral.
     *  It is only used in DIRECT mode of operation. If the requested amount of data is
     *  larger than provided by the peripheral, the function reads as many elements as
     *  possible and returns the actual number of elements read (there is no polling of
     *  the receiver).
     *  @return the actual number of elements read
     *  
     *  @param ioperipheral handle of the ioperipheral
     *  @param data pointer to the memory area where data will be stored
     *  @param size number of elements to read
     *  @param timeout timeout value (in OS ticks). This parameter is ignored and
     *  should be set to 0 when OS integration is disabled. 
     */
    size_t (*ReadDirect)(void *ioperipheral, void *data, size_t size, unsigned int timeout);

    /** TODO
     *  @return the actual number of elements read
     *
     *  @param ioperipheral handle of the ioperipheral
     *  @param data pointer to the memory area where data will be stored
     *  @param size number of elements to read
     *  @param timeout timeout value (in OS ticks). This parameter is ignored and
     *  should be set to 0 when OS integration is disabled.
     */
    size_t (*ReadDMA)(void *ioperipheral, void *data, size_t size, unsigned int timeout);

    /** 
     *  Sets the ioperipheral operating mode. The function returns the actual mode that 
     *  has been set. If the desired mode is not allowed, other mode can be sellected 
     *  (usually the default mode is the DIRECT mode).
     *  
     *  @param ioperipheral handle of the ioperipheral
     *  @param mode operating mode identifier
     */
    IODeviceMode (*SetMode)(void *ioperipheral, IODeviceMode mode);

    /** 
     *  This function is an event handler function, that may be provided by the user to
     *  act as a data sink in the EVENT-based mode of operation. Is is executed in an
     *  interrupt service routine. It accepts data generated during the receive event.
     *  The number of bytes processed by the event handler is returned/
     *  
     *  @param ioperipheral handle of the ioperipheral
     *  @param data data read during the receive event
     *  @param size number of elements read during the receive event
     */
    size_t (*ReceiveEventHandler)(void *ioperipheral, IOEventType event_type, void *data, size_t size);

    /** 
     *  This function is an event handler function, that may be provided by the user to
     *  act as a data source in the EVENT-based mode of operation. Is is executed in an
     *  interrupt service routine.  It provides data to send during the transmit event.
     *  The number of elements actually provided is returned.
     *  
     *  @param ioperipheral handle of the ioperipheral
     *  @param data pointer to the memory area, where the provided data will be stored
     *  @param size requested number of elements, that this event handler should provide
     */
    size_t (*TransmitEventHandler)(void *ioperipheral, IOEventType event_type, void *data, size_t size);

    /** 
     *  @param ioperipheral handle of the ioperipheral
     *  @param error
     */
    void (*ReceiveErrorHandler)(void *ioperipheral, uint32_t error);

    /** 
     *  @param ioperipheral handle of the ioperipheral
     *  @param error
     */
    void (*TransmitErrorHandler)(void *ioperipheral, uint32_t error);

    /** 
     *  This function writes a specified number of elements directly to the peripheral.
     *  It is only used in DIRECT mode of operation. If the requested amount of data to
     *  write is larger than what the peripheral can accept, the function writes as
     *  many elements as possible and returns the actual number of elements written
     *  (there is no polling of the transceiver)
     *  @return the actual number of elements written
     *  
     *  @param ioperipheral handle of the ioperipheral
     *  @param data pointer to source data
     *  @param size number of elements to write
     *  @param timeout timeout value (in OS ticks). This parameter is ignored and
     *  should be set to 0 when OS integration is disabled. 
     */
    size_t (*WriteDirect)(void *ioperipheral, const void *data, size_t size, unsigned int timeout);

    /**
     *  TODO
     *  @return the actual number of elements written
     *
     *  @param ioperipheral handle of the ioperipheral
     *  @param data pointer to source data
     *  @param size number of elements to write
     *  @param timeout timeout value (in OS ticks). This parameter is ignored and
     *  should be set to 0 when OS integration is disabled.
     */
    size_t (*WriteDMA)(void *ioperipheral, const void *data, size_t size, unsigned int timeout);

#if defined HAL_IO_OS_INTEGRATION && (HAL_IO_OS_INTEGRATION != 0)
    /// a semaphore used to guard access to the resource
    OSMutex lock;

    /// a semaphore used to sync the reading task with data reception
    OSSem rx_notification_sem;

    /// number of elements received in order to notify the reading task through synchronization semaphore
    size_t rx_notification_threshold;

#endif

} IOPeripheralDesc, *IOPeripheral;

/**
 *  Waits for a specified amount of data received via IOPeripheral.
 *  This function works in OS integration mode only.
 *  @param ioperipheral IOPeripheral object
 *  @param size number of elements to wait for
 *  @param timeout timeout for waiting (in OS ticks)
 */
int IOPERIPH_WaitForData(IOPeripheral ioperipheral, size_t size, unsigned int timeout);

/*@}*/
#endif /* HAL_IOPERIPHERAL_H */

