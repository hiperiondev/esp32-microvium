/*
 * @file hal_tim.c
 * @brief HAL TIM module
 *
 * @author Emiliano Gonzalez (egonzalez . hiperion @ gmail . com))
 * @version 0.1
 * @date 2023
 * @copyright MIT License
 * @see https://github.com/hiperiondev/esp32-microvium
 * @note This is based on other projects. See license files
 */

#include <stdlib.h>

#include "hal_config.h"
#include "hal_tim.h"
#include "hal_core.h"
#include "hal_diag.h"

// -----------------------------------------------------------------------------
//  TIM_Init
// -----------------------------------------------------------------------------
void TIM_Init(TIMDevice tim, uint32_t TickTimeBase) {
    int32_t i;

    DIAG_DEBUG_ASSERT_AND_EXECUTE(tim) {

        // initialize peripheral
        // this should setup the event tables as well
        tim->Init((void*) tim, TickTimeBase);

#if (HAL_TIM_USE_INTERRUPT_EVENTS)
        if (tim->IntEvents) {
            // this TIM object supports interrupt-level events
            DIAG_DEBUG_ASSERT_AND_EXECUTE(tim->IntEvents->EventTable) {
                // clear interrupt-level events associated with the timer
                tim->IntEvents->InstalledEvents = 0;
                tim->IntEvents->NextEvent.id = TIM_NO_EVENT;
                for (i = 0; i < tim->IntEvents->MaxEvents; i++) {
                    tim->IntEvents->EventTable[i].handler = NULL;
                    tim->IntEvents->EventTable[i].expires.counter_periods = 0xffffffff;
                    tim->IntEvents->EventTable[i].expires.counter_ticks = 0xffffffff;
                    tim->IntEvents->EventTable[i].next_event.id = TIM_NO_EVENT;
                }
            }
        } // if (tim->IntEvents)
#endif

#if (HAL_TIM_USE_TASK_EVENTS)
        if (tim->TskEvents) {
            // this TIM object supports task-level events
            DIAG_DEBUG_ASSERT_AND_EXECUTE(tim->IntEvents->EventTable) {
                // clear task-level events associated with the timer
                tim->TskEvents->InstalledEvents = 0;
                tim->TskEvents->NextEvent.id = TIM_NO_EVENT;
                for (i = 0; i < tim->TskEvents->MaxEvents; i++) {
                    tim->TskEvents->EventTable[i].handler = NULL;
                    tim->TskEvents->EventTable[i].expires.counter_periods = 0xffffffff;
                    tim->TskEvents->EventTable[i].expires.counter_ticks = 0xffffffff;
                    tim->TskEvents->EventTable[i].next_event.id = TIM_NO_EVENT;
                }
            }
        } // if (tim->TskEvents)
#endif

    } // assert tim
} /* TIM_Init */

// -----------------------------------------------------------------------------
//  TIM_InitEvent
// -----------------------------------------------------------------------------
TIM_EventID TIM_InitEvent(TIMDevice tim, void (*handler)(TIMDevice tim, TIM_EventID event_id, TIM_Time expire_time), TIM_EventType type) {
    int32_t i;
    TIM_EventID id;
    TIM_EventTable *Events;
    union {
        void (*v_handler)(void *tim, TIM_EventID event_id, TIM_Time expire_time);
        void (*t_handler)(TIMDevice tim, TIM_EventID event_id, TIM_Time expire_time);
    } u_handler;

    u_handler.t_handler = handler;
    id.id = TIM_NO_EVENT;
    Events = NULL;

    DIAG_DEBUG_ASSERT_AND_EXECUTE(tim) {
        DIAG_DEBUG_ASSERT_AND_EXECUTE(handler) {
            DIAG_DEBUG_ASSERT_AND_EXECUTE(((TIM_EVENT_TYPE_INTERRUPT == type) || (TIM_EVENT_TYPE_TASK == type))) {
#if (HAL_TIM_USE_INTERRUPT_EVENTS)
                if (TIM_EVENT_TYPE_INTERRUPT == type) {
                    Events = tim->IntEvents;
                }
#endif
#if (HAL_TIM_USE_TASK_EVENTS)
                if (TIM_EVENT_TYPE_TASK == type) {
                    Events = tim->TskEvents;
                }
#endif
                if (Events) {
                    CORE_EnterCritical();
                    if (Events->InstalledEvents < Events->MaxEvents) {
                        for (i = 0; i < Events->MaxEvents; i++) {
                            // search for free event slot
                            if (NULL == Events->EventTable[i].handler) {
                                // found
                                Events->EventTable[i].handler = u_handler.v_handler;
                                Events->EventTable[i].expires.counter_periods = 0xffffffff;
                                Events->EventTable[i].expires.counter_ticks = 0xffffffff;
                                Events->EventTable[i].next_event.id = TIM_NO_EVENT;
                                Events->InstalledEvents++;

                                CORE_ExitCritical();

                                id.index = i;
                                if (TIM_EVENT_TYPE_INTERRUPT == type) {
                                    id.type = 0;
                                } else {
                                    id.type = 1;
                                }

                                return id;
                            }
                        }
                        // free slot somehow not found
                        // this indicates an error since InstalledtEvents < Events->MaxEvents
                        CORE_ExitCritical();
                        return id;
                    } else {
                        // not enough free slots
                        CORE_ExitCritical();
                        return id;
                    }
                } // if (Events)

            } // assert type
        } // assert handler
    } // assert tim

    return id;

} /* TIM_InitEvent */

// -----------------------------------------------------------------------------
//  TIM_DeinitEvent
// -----------------------------------------------------------------------------
void TIM_DeinitEvent(TIMDevice tim, TIM_EventID event_id) {
    TIM_EventTable *Events;

    Events = NULL;

    DIAG_DEBUG_ASSERT_AND_EXECUTE(tim) {

#if (HAL_TIM_USE_INTERRUPT_EVENTS)
        if (event_id.type == 0) {
            // interrupt-level event
            Events = tim->IntEvents;
        }
#endif
#if (HAL_TIM_USE_TASK_EVENTS)
        if (event_id.type == 1) {
            // task-level event
            Events = tim->TskEvents;
        }
#endif
        if (Events) {
            DIAG_DEBUG_ASSERT_AND_EXECUTE(((event_id.index >= 0) && (event_id.index < Events->MaxEvents))) {
                CORE_EnterCritical();
                // this deinitializes the event, by disconnecting the event handler func
                Events->EventTable[event_id.index].handler = NULL;
                Events->InstalledEvents--;
                // TODO: CRITICAL! remove event from queue
                CORE_ExitCritical();
            }
        } // if (Events)
    } // assert tim

} /* TIM_DeinitEvent */

// -----------------------------------------------------------------------------
//  TIM_ScheduleEvent
// -----------------------------------------------------------------------------
int TIM_ScheduleEvent(TIMDevice tim, TIM_EventID event_id, TIM_Time delta_time) {
    TIM_Time time;
    uint32_t ticks;
    TIM_Capabilities caps;

    DIAG_DEBUG_ASSERT_AND_EXECUTE(tim) {
        // calculate absolute time of event using current time and delta
        time = tim->GetTimeElapsed(tim);
        TIM_GetCapabilities(tim, &caps);
        time.counter_periods += delta_time.counter_periods;
        ticks = time.counter_ticks;
        time.counter_ticks += delta_time.counter_ticks;
        // check if variable overflow occurred
        // This might be checked this way since we're adding unsigned integers
        // and the modulo2 behavior is well defined (won't work with signed integers)
        if (time.counter_ticks < ticks) {
            time.counter_periods++;
        }
        // check counter_ticks overflow - requires knowledge about counter range
        while (time.counter_ticks > caps.CounterRange) {
            time.counter_periods++;
            time.counter_ticks -= (caps.CounterRange + 1);
        }
        return TIM_ScheduleEventAt(tim, event_id, time);
    }
    return -1;

} /* TIM_ScheduleEvent */

// -----------------------------------------------------------------------------
//  TIM_ScheduleEventAt
// -----------------------------------------------------------------------------
int TIM_ScheduleEventAt(TIMDevice tim, TIM_EventID event_id, TIM_Time abs_time) {
    int32_t i;
    TIM_Event *event;
    TIM_EventTable *Events;
    int found;

    Events = NULL;

    DIAG_DEBUG_ASSERT_AND_EXECUTE(tim) {

#if (HAL_TIM_USE_INTERRUPT_EVENTS)
        if (event_id.type == 0) {
            // interrupt-level event
            Events = tim->IntEvents;
        }
#endif
#if (HAL_TIM_USE_TASK_EVENTS)
        if (event_id.type == 1) {
            // task-level event
            Events = tim->TskEvents;
        }
#endif
        DIAG_DEBUG_ASSERT_AND_EXECUTE(((event_id.index >= 0) && (event_id.index < Events->MaxEvents))) {
            DIAG_DEBUG_ASSERT_AND_EXECUTE(Events->EventTable[event_id.index].handler){
            CORE_EnterCritical();
            if (Events->NextEvent.id != TIM_NO_EVENT) {
                // some events are already scheduled
                // insert event into the queue
                event = &Events->EventTable[Events->NextEvent.index];
                i = 0;
                while (i < Events->InstalledEvents) {
                    if ((abs_time.counter_periods < event->expires.counter_periods) ||
                            ((abs_time.counter_periods == event->expires.counter_periods) && (abs_time.counter_ticks < event->expires.counter_ticks)))
                    {
                        found = 1;
                        // insert event here
                        Events->EventTable[event_id.index].expires = abs_time;
                        Events->EventTable[event_id.index].next_event = event->next_event;
                        event->next_event = event_id;
                        if (i == 0) {
                            // this event is the first in queue
                            Events->NextEvent = event_id;
                            tim->ScheduleEvent(tim, abs_time);
                        }
                        CORE_ExitCritical();
                        return 0;
                    } else {
                        // check next event in queue
                        if (event->next_event.id != TIM_NO_EVENT) {
                            event = &Events->EventTable[event->next_event.index];
                        } else {
                            // insert event at the end of the queue
                            Events->EventTable[event_id.index].expires = abs_time;
                            Events->EventTable[event_id.index].next_event.id = TIM_NO_EVENT;
                            event->next_event = event_id;
                            CORE_ExitCritical();
                            return 0;
                        }
                    }
                    i++;
                } // while
            } else {
                // this event is the first one to be scheduled
                Events->EventTable[event_id.index].expires = abs_time;
                Events->EventTable[event_id.index].next_event.id = TIM_NO_EVENT;
                Events->NextEvent = event_id;
                tim->ScheduleEvent(tim, abs_time);
                CORE_ExitCritical();
                return 0;
            }
            CORE_ExitCritical();

        } // assert handler
    } // assert index
} // assert tim

    return -1;

} /* TIM_ScheduleEventAt */

// -----------------------------------------------------------------------------
//  TIM_SetEventType
// -----------------------------------------------------------------------------
TIM_EventID TIM_SetEventType(TIMDevice tim, TIM_EventID event_id, TIM_EventType type) {
    union {
        void (*v_handler)(void *tim, TIM_EventID event_id, TIM_Time expire_time);
        void (*t_handler)(TIMDevice tim, TIM_EventID event_id, TIM_Time expire_time);
    } handler;
    TIM_Time time;
    TIM_EventID id;

    id.id = TIM_NO_EVENT;

    DIAG_DEBUG_ASSERT_AND_EXECUTE(tim) {
#if (HAL_TIM_USE_INTERRUPT_EVENTS)
        if (type == TIM_EVENT_TYPE_INTERRUPT) {
            if (0 == event_id.type) {
                // the provided event is already an interrupt-level event
                return event_id;
            } else {
                // TODO:interrupt-level event must be changed into task-level event
                CORE_EnterCritical();
                // save event handler and schedule time
                handler.v_handler = tim->IntEvents->EventTable[event_id.index].handler;
                time = tim->IntEvents->EventTable[event_id.index].expires;
                TIM_DeinitEvent(tim, event_id);
                id = TIM_InitEvent(tim, handler.t_handler, type);
                CORE_ExitCritical();
                if (id.id != TIM_NO_EVENT) {
                    // re-schedule the event
                    TIM_ScheduleEventAt(tim, id, time);
                }
            }
        }
#endif
#if (HAL_TIM_USE_TASK_EVENTS)
        if (type == TIM_EVENT_TYPE_TASK) {
            if (1 == event_id.type) {
                // the provided event is already a task-level event
                return event_id;
            } else {
                // TODO:task-level event must be changed into interrupt-level event
            }
        }
#endif
    }

    return id;
} /* TIM_SetEventType */

// -----------------------------------------------------------------------------
//  TIM_GetEventType
// -----------------------------------------------------------------------------
TIM_EventType TIM_GetEventType(TIMDevice tim, TIM_EventID event_id) {
    // just to avoid compiler warnings
    //tim = tim;

    if (0 == event_id.type) {
        return TIM_EVENT_TYPE_INTERRUPT;
    } else {
        return TIM_EVENT_TYPE_TASK;
    }

    return TIM_EVENT_TYPE_NO_TYPE;

} /* TIM_GetEventType */

// -----------------------------------------------------------------------------
//  TIM_GetTimeToEvent
// -----------------------------------------------------------------------------
uint32_t TIM_GetTimeToEvent(TIMDevice tim, TIM_EventID event_id) {
//  TIM_Event *eTable;
//  if(tim && (handler_id >= 0) && (handler_id < (TIM_MAX_NUM_INTERRUPT_EVENTS + TIM_MAX_NUM_TASK_EVENTS))) {
//      eTable = &tim->Events->Table[handler_id];
//
//      // if there is correct handler
//      if(eTable->handler) {
//          return(eTable->delay - tim->GetTimeFromLastIrq(tim));
//      }
//  }
    return (0);
} /* TIM_GetTimeToEvent */

// -----------------------------------------------------------------------------
//  TIM_TaskEventProc
// -----------------------------------------------------------------------------
void TIM_TaskEventProc(TIMDevice tim) {
    TIM_Time time;
    TIM_EventID id;
    TIM_Event *event;
    int keep_running;

    DIAG_DEBUG_ASSERT_AND_EXECUTE(tim) {
        do {
            keep_running = 0;
            if (tim->TskEvents->NextEvent.id != TIM_NO_EVENT) {
                // there are some events scheduled in the queue
                event = &tim->TskEvents->EventTable[tim->TskEvents->NextEvent.index];
                time = tim->GetTimeElapsed(tim);
                if ((time.counter_periods > event->expires.counter_periods)
                        || ((time.counter_periods == event->expires.counter_periods) && (time.counter_ticks >= event->expires.counter_ticks))) {
                    // event expired
                    time = event->expires;
                    event->expires.counter_periods = 0;
                    event->expires.counter_ticks = 0;
                    id.id = tim->TskEvents->NextEvent.id;
                    tim->TskEvents->NextEvent.id = event->next_event.id;
                    event->handler(tim, id, time);
                    keep_running = 1;
                }
            }
        } while (keep_running);
    }
} /* TIM_TaskEventProc */

// -----------------------------------------------------------------------------
//  TIM_InterruptProc
// -----------------------------------------------------------------------------
void TIM_InterruptProc(TIMDevice tim) {
    TIM_Time time;
    TIM_EventID id;
    TIM_Event *event;
    int keep_running;
    int events_executed;

    events_executed = 0;

    do {
        keep_running = 0;
        if (tim->IntEvents->NextEvent.id != TIM_NO_EVENT) {
            // there are some events scheduled in the queue
            event = &tim->IntEvents->EventTable[tim->IntEvents->NextEvent.index];
            time = tim->GetTimeElapsed(tim);
            if ((time.counter_periods > event->expires.counter_periods)
                    || ((time.counter_periods == event->expires.counter_periods) && (time.counter_ticks >= event->expires.counter_ticks))) {
                // event expired
                time = event->expires;
                event->expires.counter_periods = 0;
                event->expires.counter_ticks = 0;
                id.id = tim->IntEvents->NextEvent.id;
                tim->IntEvents->NextEvent.id = event->next_event.id;
                event->handler(tim, id, time);
                keep_running = 1;
            } else {
                if (0 == events_executed) {
                    // re-schedule nearest event
                    tim->ScheduleEvent(tim, event->expires);
                    //TIM_ScheduleEventAt(tim, tim->IntEvents->NextEvent, event->expires);
                }
            }
        }
    } while (keep_running);
} /* TIM_InterruptEventProc */
