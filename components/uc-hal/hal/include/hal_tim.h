/*
 * @file hal_tim.h
 * @brief HAL TIM module API
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include "hal_port_tim.h"

#ifndef HAL_TIM_H
#define HAL_TIM_H

#include <stdint.h>

/** \defgroup hal_tim TIM module
 * 
 * <b>Introduction.</b>
 * 
 * TIM module provides time-based functionalities such as free running counter and time-based 
 * events. TIM module usually uses internal peripheral timers and timer interrupts
 * to implement periodical time ticks, that become time units. These interrupts also trigger 
 * the application-defined events. The events can be executed inside the trigger interrupt
 * service routine (so called interrupt-level events) or in the background task (task-level
 * events). TIM module uses TIMDevice as timer abstraction. Hardware-specific ports are responsible
 * for providing such objects.
 *  
 * <b>Free running counter.</b>
 * 
 * One of the main TIM functionalities is to provide a simple counter that will be incremented 
 * periodically with as much precision as possible. This counter is started during timer 
 * initialization by a call to \ref TIM_Init. The current value of the counter can be obtained
 * by a call to \ref TIM_GetTimeElapsed.
 * 
 * <b>Time-based events.</b>
 * 
 * Application can generate time-based events. An event is signaled when a specified amount
 * of time passes. Time based events use free running counter to measure such time periods.
 * When event is signaled an application-defined function (an event handler) can be called.
 * 
 * To create new event and connect an event handler to it the application must call
 * \ref TIM_InitEvent. This call returns event_id that is used to identify the event in the
 * scope of a single TIMDevice.
 *
 * To schedule an event two functions can be used: \ref TIM_ScheduleEvent and \ref TIM_ScheduleEventAt.
 *
 * There are two types of events that differ in a way the event handlers are called:
 * - The "interrupt-level" events make the event handler functions execute directly in the interrupt
 *   service routine (ISR) that triggered the event. Such events must be used with care, and should
 *   be as short as possible. 
 * - The "task-level" events do not call the event handlers from inside the ISR. Instead, they just mark
 *   that the event was triggered. The handler is executed during the next call to the
 *   \ref TIM_TaskEventProc, which can be in turn called periodically in a OS task or inside some main loop.
 * 
 * The main difference between these two types of event is the response time precision. 
 * The response time for "interrupt-level" events is usually shorter, because the event handlers are called
 * directly by the triggering interrupt. These are suited for precise time actions. The "task-level"
 * events usually add some time overhead before the next \ref TIM_TaskEventProc is called. These
 * are suited for less time-critical jobs. 
 * 
 * The type of event is declared during event setup (a call to \ref TIM_InitEvent) but
 * it can be changed any time by the \ref TIM_SetEventType. The \ref TIM_GetEventType returns
 * the current type of an event. 
 *   
 * <b>Timer setup.</b>
 * 
 * To setup the timer the application must call \ref TIM_Init and provide the timer time base.
 * This time base defines the timer resolution. It's the period between two timer ticks, expressed
 * in nano-seconds. So for example:
 * \code
 * TIM_Init(TIM1, 1000);
 * \endcode
 * would setup TIM1 with 1us resolution.
 *
 * <b>Using TIM events</b>
 *
 * Let's toggle a LED using events with HAL_TIM1 timer. Let's say we want to toggle every 200ms.
 * To do that we setup HAL_TIM1 to count every 1ms.
 * 
 * \code
 * // setup timer for 1000000ns = 1ms resolution
 * TIM_Init(HAL_TIM1, 1000000);
 * \endcode
 * 
 * Now, let's create an interrupt-level event and schedule it.
 *
 * \code
 * TIM_EventID led_event;
 * TIM_Time t;
 *
 * // initialize event
 * led_event = TIM_InitEvent(HAL_TIM1, event_handler, TIM_EVENT_TYPE_INTERRUPT);
 *
 * // first event after 100ms from now
 * t.counter_periods = 0;
 * t.counter_ticks = 100;
 * TIM_ScheduleEvent(HAL_TIM1, led_event, t);
 * \endcode
 * 
 * We must define the event handler:
 * 
 * \code
 * void event_handler(TIMDevice tim, TIM_EventID event_id, TIM_Time expire_time)
 * {
 *     // toggle LED
 *     HAL_GPIO_TogglePin(LED1);
 *     // re-schedule event
 *     expire_time->counter_ticks += 200;
 *     TIM_ScheduleEventAt(tim, event_id, expire_time);
 * }
 * \endcode
 * 
 * To use the same event as task-level event, the application must periodically call
 * \ref TIM_TaskEventProc, for example in OS task:
 * 
 * \code
 * void TASK_TIM(void *pvParameters)
 * {
 *  while(1) {
 *      TIM_TaskEventProc(HAL_TIM);
 *  }   
 * }
 * \endcode
 * 
 * 
 * <b>Module configuration.</b>
 * 
 * To enable the TIM module, HAL_ENABLE_TIM definition must be set to 1, 
 * in hal_config.h. The \ref HAL_TIM_USE_INTERRUPT_EVENTS and
 * \ref HAL_TIM_USE_TASK_EVENTS enable or disable interrupt-level events
 * and task-level events.
 */

/*@{*/

#ifndef HAL_TIM_USE_INTERRUPT_EVENTS
/// Enables (1) or disables (0) interrupt level events
#define HAL_TIM_USE_INTERRUPT_EVENTS    1
#endif

#ifndef HAL_TIM_USE_TASK_EVENTS
/// Enables (1) or disables (0) task level events
#define HAL_TIM_USE_TASK_EVENTS         1
#endif

/// An identifier representing uninitialized (empty) event
#define TIM_NO_EVENT                    0xffffffff

// -----------------------------------------------------------------------------
//  PUBLIC TYPEDEFS
// -----------------------------------------------------------------------------

/** Event identifier type */
typedef union {
    uint32_t id;
    struct {
        /// event index
        uint32_t index :31;
        /// event type: 0 - interrupt-level event, 1 - task-level event
        uint32_t type :1;
    };
} TIM_EventID;

/** Type representing time in TIM module */
typedef struct {
    // absolute number of counter periods
    uint32_t counter_periods;
    // absolute number of ticks
    uint32_t counter_ticks;
} TIM_Time;

/** Descriptor of an event */
typedef struct {
    /// time at which event expires
    TIM_Time expires;
    /// event handler function.
    void (*handler)(void *tim, TIM_EventID event_id, TIM_Time expire_time);
    /// id of the next event
    TIM_EventID next_event;
} TIM_Event;

/** Timer events description */
typedef struct {
    /// maximal number of events that can be installed (this is the size of EventTable)
    int32_t MaxEvents;
    /// number of installed events
    int32_t InstalledEvents;
    /// index of the next interrupt-level event
    TIM_EventID NextEvent;
    /// event table, note that event_id holds an index to this table
    TIM_Event *EventTable;
} TIM_EventTable;

/** Possible event types */
typedef enum {
    /// Uninitialized type of event
    TIM_EVENT_TYPE_NO_TYPE = -1,
    /// This type indicates that the event handler is called from within an interrupt service routine.
    TIM_EVENT_TYPE_INTERRUPT = 0,
    /// This type indicates, that the event handler is called from a task (and NOT directly in an interrupt service routine).
    TIM_EVENT_TYPE_TASK = 1
} TIM_EventType;

/** Description of hardware timer capabilities */
typedef struct {
    /// Lowest possible time base for a timer tick (in nanoseconds) - for highest timer resolution
    uint32_t MinTickBase;
    /// Highest possible time base for a timer tick(in nanoseconds) - for widest time range
    uint32_t MaxTickBase;
    /// Hardware counter range. This would be for example 0x0000ffff for 16bit timers
    uint32_t CounterRange;
} TIM_Capabilities;

/** Descriptor of a TIM object */
typedef struct {
    /**
     * Specifies the time between timer ticks in nanoseconds.
     */
    uint32_t TickTimeBase;

    /**
     * Table of interrupt-level events.
     */
    TIM_EventTable *IntEvents;
    /**
     * Table of task-level events.
     */
    TIM_EventTable *TskEvents;

    /** 
     *  Initializes specified timer. The timer is started immediately.
     *  This function is never called from an interrupt service routine.
     *  
     *  @param TIM timer handle
     *  @param TickTimeBase time between timer ticks in nanoseconds (timer resolution)
     */
    void (*Init)(void *TIM, uint32_t TickTimeBase);

    /** 
     *  Deinitializes a specified timer. This function should stop the running timer.
     *  This function is never called from an interrupt service routine.
     *  
     */
    void (*Deinit)(void *TIM);

    /** 
     *  Gets the current time elapsed since the timer was initialized and started running.
     *  This function my be called from an interrupt service routine.
     *  
     *  @param TIM timer handle
     *  @return time elapsed since the timer was initialized
     */
    TIM_Time (*GetTimeElapsed)(void *TIM);

    /** 
     *  Resets timer's tick counter.
     *  
     *  @param TIM timer handle
     */
    void (*ResetCounter)(void *TIM);

    /**
     *  Gets hardware capabilities of a timer
     *  @param TIM timer handle
     *  @param caps pointer to a structure, where timer capabilities will be stored
     */
    void (*GetCapabilities)(void *TIM, TIM_Capabilities *caps);

    /**
     *  Gets the difference between the selected TickTimeBase and the actual value.
     *  This difference is due to hardware capabilities of the timer, and it is
     *  expressed in nanoseconds.
     *  @param TIM TIM object
     *  @param TickTimeBase selected time base
     *  @return selected time base minus actual time base that can be realized by hardware (in nanoseconds)
     */
    int32_t (*GetError)(void *TIM, uint32_t TickTimeBase);

    /**
     *  Schedules a call to TIM_InterruptProc at the specified time.
     */
    int (*ScheduleEvent)(void *TIM, TIM_Time time);

} TIMDeviceDesc, *TIMDevice;

// -----------------------------------------------------------------------------
//  PUBLIC API
// -----------------------------------------------------------------------------

/** 
 *  Initializes the specified timer object and starts the timer.
 *  
 *  @param tim timer handle
 *  @param TickTimeBase time between timer ticks in nanoseconds
 */
void TIM_Init(TIMDevice tim, uint32_t TickTimeBase);

/** 
 *  Deinitializes specified timer object. Stops the running timer.
 *  
 *  @param TIM timer handle
 */
#define TIM_Deinit(TIM)                                     (TIM)->Deinit(TIM)

/** 
 *  Returns the number of timer ticks elapsed since the timer was initialized and started running.
 *  
 *  @param TIM timer handle
 */
#define TIM_GetTimeElapsed(TIM)                             (TIM)->GetTimeElapsed(TIM)

/** 
 *  Resets specified timer's counter
 *  
 *  @param TIM timer handle
 */
#define TIM_ResetCounter(TIM)                               (TIM)->ResetCounter(TIM)

/** 
 *  Resets specified timer's counter
 *
 *  @param TIM timer handle
 *  @param Caps pointer to description structure
 */
#define TIM_GetCapabilities(TIM, Caps)                      (TIM)->GetCapabilities(TIM, Caps)

/**
 *  Resets specified timer's counter
 *
 *  @param TIM timer handle
 *  @param TickTimeBase selected time base
 *  @return selected time base minus actual time base that can be realized by hardware (in nanoseconds)
 */
#define TIM_GetError(TIM, TickTimeBase)                     (TIM)->GetError(TIM, TickTimeBase)

/**
 *  Initializes a new event and connects it's event handler.
 *  @param tim timer handle
 *  @param handler event handler function
 *  @param type event type - this can be TIM_EVENT_TYPE_INTERRUPT or TIM_EVENT_TYPE_TASK
 *  @return ID of a new event associated with timer or -1 if initialization failed
 */
TIM_EventID TIM_InitEvent(TIMDevice tim, void (*handler)(TIMDevice tim, TIM_EventID event_id, TIM_Time expire_time), TIM_EventType type);

/**
 *  Deinitializes an event. The provided event_id will be reused.
 *  @param tim timer handle
 *  @param event_id event id returned by a previous call to \ref TIM_InitEvent or \ref TIM_SetEventType
 */
void TIM_DeinitEvent(TIMDevice tim, TIM_EventID event_id);

/**
 *  Schedules an event to be triggered after a specified amount of time.
 *  @param tim timer handle
 *  @param event_id event id returned by a previous call to \ref TIM_InitEvent or \ref TIM_SetEventType
 *  @param delta_time - time after which event will be triggered
 *  @return 0 if schedule succeeded or -1 if failed
 */
int TIM_ScheduleEvent(TIMDevice tim, TIM_EventID event_id, TIM_Time delta_time);

/**
 *  Schedules an event to be triggered after a specified amount of time.
 *  @param tim timer handle
 *  @param event_id event id returned by a previous call to \ref TIM_InitEvent or \ref TIM_SetEventType
 *  @param abs_time - absolute time at which event will be triggered
 *  @return 0 if schedule succeeded or -1 if failed
 */
int TIM_ScheduleEventAt(TIMDevice tim, TIM_EventID event_id, TIM_Time abs_time);

/** 
 *  Sets event type
 *  
 *  @param tim timer handler
 *  @param event_id ID of event that's type we want to set
 *  @param type new type of handler
 *  @return new Event ID
 */
TIM_EventID TIM_SetEventType(TIMDevice tim, TIM_EventID event_id, TIM_EventType type);

/** 
 *  Returns event type
 *  
 *  @param tim timer handler
 *  @param event_id ID of event that's type we want to get
 *  @return Event type
 */
TIM_EventType TIM_GetEventType(TIMDevice tim, TIM_EventID event_id);

/** 
 *  Returns time left to call of specified function
 *  
 *  @param tim timer handler
 *  @param event_id ID of event that's "time to Event" we want to get
 *  @return time to Event
 */
uint32_t TIM_GetTimeToEvent(TIMDevice tim, TIM_EventID event_id);

/** 
 *  Calls each handler connected to specified timer that need to be called. This
 *  function have to be always called in some loop (in task or "main").
 *  
 *  @param tim timer handler
 */
void TIM_TaskEventProc(TIMDevice tim);

/** 
 *  Calls each handler connected to specified timer that need to be called. This
 *  function have to be called in specified interrupt.
 *  
 *  @param tim timer handler
 */
void TIM_InterruptProc(TIMDevice tim);

/*@}*/

#endif /* HAL_TIM_H */
