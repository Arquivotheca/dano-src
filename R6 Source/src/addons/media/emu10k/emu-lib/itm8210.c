/*****************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
*****************************************************************************/

/*****************************************************************************
*
* @doc INTERNAL
* @module itm8210.c | The Interval Timer Manager implementation.
*
* Theory of operation:  We maintain a linked list of Callback Information,
* with each structure containing the time at which is to be triggered.
* @iex 
* Revision History:
*
* Person            Date			Reason
* ---------------	------------    --------------------------------------
* John Kraft		July 14, 98		Created.
*
*****************************************************************************/

/******************
 * Include files
 ******************/

#include "datatype.h"
#include "itm8210.h"
#include "itm8210d.h"
#include "os8210.h"
#include "dbg8210.h"

/*******************
 * Definitions
 *******************/
#define TIMER_FREQUENCY 50  /* Interrupt every 50 sample periods, or ~ 1 ms */

#define ITM_ID_MAGIC    0xDECADEL
#define MAKE_ITMID(_x)  ((ITM_ID_MAGIC << 8) | (_x))
#define GET_ITMINDEX(_x) ((_x) & 0xFF)
#define IS_VALID_ITMID(_x) ( (((_x) >> 8) == ITM_ID_MAGIC)   && \
                             (GET_ITMINDEX(_x) < MAX_CHIPS)  && \
                             (gItmStates[GET_ITMINDEX(_x)].dwInUse != 0) )
#define FIND_STATE(_x) (IS_VALID_ITMID(_x) ? &gItmStates[GET_ITMINDEX(_x)] : NULL)

#define MAKE_HANDLE(_x) ((ITMHANDLE) (_x))
#define IS_VALID_HANDLE(_x) (((DWORD)(_x) != 0) && \
                             (((stEvent*)(_x))->dwMagic == EVENT_MAGIC))
#define FIND_EVENT(_x) (IS_VALID_HANDLE(_x) ? (stEvent*) (_x) : NULL)



#define READ_TIME(_pstate) (L8010SERegRead(_pstate->halid, WC) >> 6)

#define ITMSF_ABSOLUTE      0x1
#define ITMSF_DONT_CASCADE  0x2

#define EVENT_MAGIC         0xEEEE0000

#define BASE_TIMER_FREQ     48000

#define MIN_TIMER_VALUE     5

// Do not define this unless you're testing the variable rate timer code
#define USE_VAR_TIMER

/********************
 * Structures
 ********************/

/* @struct stEvent |
 *  This structure contains information needed for a particular
 *  callback.  They are linked into a time-ordered list for
 *  ease of dispatch.
 */

typedef struct _event {
    DWORD                 dwMagic;
    ITMTIME               time;
    ITMSCHEDINFO          schedinfo;
    DWORD                 dwInvocationCount;
    struct _stITMState    *pstate;
    struct _event        *pNextEvent;
    struct _event        *pNextCallbackEvent;
    BOOL                  bOnCallbackList;
} stEvent;



/* @struct stITMState | 
 *  This structure contains data relevant to a particular ITM
 *  instance.  One ITM stITMState structure exists for every 8010 
 *  chip in the system.
 */
typedef struct _stITMState {
    ITMID           itmid;
    HALID           halid;
    IPID            ipid;
    DWORD           dwInUse;
    stEvent        *pEventList;
    stEvent        *pCallbackEventList;
    stEvent        *pFreeEventList;
    DWORD           dwLastClockValue;
    ITMTIME         timeNextCallback;
    ITMTIME         timeLast;
    IPHANDLE        iphandle;
    OSTIMEOUTHANDLE hTimeout;
} stITMState;


/****************************************************************************
 *  Global Variables
 ****************************************************************************/

stITMState   gItmStates[MAX_CHIPS];
WORD         gwItmCount;


/****************************************************************************
 *  Static function declarations
 ****************************************************************************/

static stEvent *_allocEvent(stITMState*);
static void     _freeEvent(stITMState*, stEvent*);

static void    _computeInvocationTime(stEvent*, ITMTIME*);
static void    _addToEventList(stEvent*);
static BOOL    _removeFromEventList(stEvent*);
static EMUSTAT _cancelEvent(stEvent*);
static void    _updateTimer(stITMState *, ITMTIME*, BOOL bScanList);

static BOOL    _itimerISRCallback(IPHANDLE, enIPType, DWORD, unIPReply*);
static BOOL    _itimerCallback(IPHANDLE, enIPType, DWORD, unIPReply*);
static void    _timeUpdateCallback(ITMID itmid);



/****************************************************************************
 *  Chip Discovery Functions
 ****************************************************************************/


/* 
 * Initialize the ITM primary state structure 
 */
EMUAPIEXPORT EMUSTAT
itmInit()
{
    gwItmCount = 0;
    memset(gItmStates, 0x0, sizeof(gItmStates));

    return SUCCESS;
}


/*
 * Allocate a state structure for a particular chip.
 */
EMUAPIEXPORT EMUSTAT
itmDiscoverChip(HALID halid, IPID ipid, ITMID *retID)
{
    DWORD         i;         /* Index variable for searching states */
    stITMState   *pstate;    /* Pointer to new state entry */

    /* Clear this for paranoia's sake */
    *retID = 0;

    /* Find a free entry in the states array */
    for (i = 0; i < MAX_CHIPS; i++) 
        if (!gItmStates[i].dwInUse) 
            break;
    if (i == MAX_CHIPS) 
        RETURN_ERROR(ITMERR_INIT_FAILED);

    /* Initialize the state structure */
    pstate = &gItmStates[i];
    pstate->itmid         = MAKE_ITMID(i);
    pstate->dwInUse       = 0xBEEFFACEL;
    pstate->halid         = halid;
    pstate->ipid          = ipid;
    pstate->pEventList = NULL;
    pstate->pFreeEventList = NULL;
    pstate->pCallbackEventList = NULL;
    pstate->dwLastClockValue = 0;
    pstate->timeNextCallback.dwLoTime = 0xFFFFFFFF;
    pstate->timeNextCallback.dwHiTime = 0xFFFFFFFF;
    pstate->timeLast.dwLoTime = 0;
    pstate->timeLast.dwHiTime = 0;
    pstate->iphandle = 0;

    /* Schedule a periodic callback for updating the clock */
    if (osScheduleTimeout(5000, _timeUpdateCallback, pstate->itmid, 
        &pstate->hTimeout) != SUCCESS) {
        RETURN_ERROR(ITMERR_INIT_FAILED);
    }

    *retID = pstate->itmid;
    return SUCCESS;
}


/* 
 * Shut down the interval timer manager.
 */
EMUAPIEXPORT EMUSTAT
itmUndiscoverChip(ITMID itmid)
{
    stITMState     *pstate;
    stEvent *pCurrEvent;

    if ((pstate = FIND_STATE(itmid)) == NULL) 
        RETURN_ERROR(ITMERR_BAD_ID);

    /* Shut down the interrupt */
    if (pstate->iphandle)
        ipUnregisterCallback(pstate->iphandle);

    /* Clean up the callback list */
    pCurrEvent = pstate->pEventList;
    while (pCurrEvent != NULL) {
        stEvent *pNextEvent = pCurrEvent->pNextEvent;
        _freeEvent(pstate, pCurrEvent);
        pCurrEvent = pNextEvent;
    }

    osUnscheduleTimeout(pstate->hTimeout);

    /* Mark the state as unused */
    pstate->dwInUse = 0;

    return SUCCESS;
}


/*
 * Schedule a callback.  Scheduling entails computing the time at
 * which the callback should be invoked and then adding it to the
 * callback list.
 */

EMUAPIEXPORT EMUSTAT
itmScheduleCallback(ITMID itmid, ITMSCHEDINFO *psi, ITMHANDLE *pRetHandle)
{
    stEvent *pNewEvent;
    stITMState     *pstate;

    /* Check the parameters for errors */
    if ((pstate = FIND_STATE(itmid)) == NULL)
        RETURN_ERROR(ITMERR_BAD_ID);

    if ((pNewEvent = _allocEvent(pstate)) == NULL)
        RETURN_ERROR(ITMERR_NO_MEMORY);

    if (psi == NULL || pRetHandle == NULL)
        RETURN_ERROR(ITMERR_BAD_PARAM);

    /* Set up the new callback info */
    pNewEvent->dwMagic           = EVENT_MAGIC;
    pNewEvent->schedinfo         = *psi;
    pNewEvent->dwInvocationCount = 0;
    pNewEvent->pstate            =  pstate;
    pNewEvent->pNextEvent        = NULL;
    pNewEvent->pNextCallbackEvent= NULL;
    pNewEvent->bOnCallbackList   = FALSE;

    /* Okay stick the the callback info to the callback list */
    _addToEventList(pNewEvent);

    *pRetHandle = (ITMHANDLE) pNewEvent;
    return SUCCESS;
}


/*
 * Cancel a callback
 */
EMUAPIEXPORT EMUSTAT
itmCancelCallback(ITMHANDLE itmh)
{
    stEvent *pevent = FIND_EVENT(itmh);
    if (pevent == NULL)
        RETURN_ERROR(ITMERR_BAD_HANDLE);

    return _cancelEvent((stEvent*) itmh);
}


/* 
 * Return the current time.
 */
EMUAPIEXPORT EMUSTAT
itmGetCurrentTime(ITMID itmid, ITMTIME *ptime)
{
    stITMState *pstate;
    DWORD       dwClock;
    DWORD       dwTimeChange;
    DWORD       dwOldLoTime;

    if ((pstate = FIND_STATE(itmid)) == NULL)
        RETURN_ERROR(ITMERR_BAD_ID);

    /* We need to disable interrupts here to 
     * prevent the interrupt handler from reentering
     * this code.  It makes extensive use of itmGetCurrentTime 
     * and we need to insure that the clock update code below
     * is performed atomically.
     */
    IDISABLE();

    dwClock = READ_TIME(pstate);

    /* Check to see if has wrapped around */
    if (dwClock < pstate->dwLastClockValue)
        /* The clock has wrapped.  Compute total change. */
        dwTimeChange = (0xFFFFF - pstate->dwLastClockValue) + dwClock;
    else 
        dwTimeChange = (dwClock - pstate->dwLastClockValue);

    /* Check to see if the least significant DWORD of the time has wrapped */
    dwOldLoTime = pstate->timeLast.dwLoTime;
    pstate->timeLast.dwLoTime += dwTimeChange;
    if (pstate->timeLast.dwLoTime < dwOldLoTime) {
        pstate->timeLast.dwHiTime++;
    }

    pstate->dwLastClockValue = dwClock;

    IENABLE();

    if (ptime == NULL) {
        RETURN_ERROR(ITMERR_BAD_PARAM);
    } else {
        *ptime = pstate->timeLast;
    }

    return SUCCESS;
}


/*
 * Return the time at which the event will trigger.
 */
EMUAPIEXPORT EMUSTAT
itmGetTriggerTime(ITMHANDLE itmh, ITMTIME *ptime)
{
    stEvent *pevent = FIND_EVENT(itmh);

    if (pevent == NULL)
        RETURN_ERROR(ITMERR_BAD_HANDLE);

    if (ptime == NULL)
        RETURN_ERROR(ITMERR_BAD_PARAM);

    /* We need to disable interrupts here to keep the
     * interrupt handler from changing event's trigger
     * time while we're in the process of transferring
     * it.  */
    IDISABLE();
    *ptime = pevent->time;
    IENABLE();

    return SUCCESS;
}


/*
 * Change the scheduling info 
 */
EMUAPIEXPORT EMUSTAT
itmChangeSchedInfo(ITMHANDLE itmh, ITMSCHEDINFO *psi)
{
    stEvent *pevent = FIND_EVENT(itmh);

    if (pevent == NULL)
        RETURN_ERROR(ITMERR_BAD_HANDLE);

    if (psi == NULL)
        RETURN_ERROR(ITMERR_BAD_PARAM);

    /* We want the structure copy to be performed
     * as an atomic operation.  */
    IDISABLE();
    pevent->schedinfo = *psi;
    IENABLE();

    return SUCCESS;
}


/*
 * Add time 1 to time2, resulting in time3.
 */
EMUAPIEXPORT ITMTIME
itmAddTimes(ITMTIME *ptime1, ITMTIME *ptime2)
{
    ITMTIME time3;

    time3.dwLoTime = ptime1->dwLoTime + ptime2->dwLoTime;
    time3.dwHiTime = ptime1->dwHiTime + ptime2->dwHiTime;

    /* Check for a carry */
    if (time3.dwLoTime < ptime1->dwLoTime) 
        time3.dwHiTime++;

    return time3;
}


/* 
 * Subtract time 2 from time 1, resulting in time3
 */
EMUAPIEXPORT ITMTIME
itmSubtractTimes(ITMTIME *ptime1, ITMTIME *ptime2)
{
    ITMTIME time3;

    time3.dwLoTime = ptime1->dwLoTime - ptime2->dwLoTime;
    time3.dwHiTime = ptime1->dwHiTime - ptime2->dwHiTime;

    /* Check for borrow */
    //if ((time3.dwLoTime & 0x80000000) != (ptime1->dwLoTime & 0x80000000)) 
    if (ptime1->dwLoTime < ptime2->dwLoTime)
        time3.dwHiTime--;

    return time3;
}


/*
 * Determine whether time1 is less than or equal to time2
 */
EMUAPIEXPORT BOOL
itmIsTime1LessThanOrEqualToTime2(ITMTIME *ptime1, ITMTIME *ptime2)
{
    /* Check the high DWORD first */
    if (ptime1->dwHiTime < ptime2->dwHiTime)
        return TRUE;
    else if (ptime1->dwHiTime == ptime2->dwHiTime) {
        if (ptime1->dwLoTime <= ptime2->dwLoTime)
            return TRUE;
        else
            return FALSE;
    } else
        return FALSE;
}


/*
 * Determine if ptime1 is equal to ptime2.
 */
EMUAPIEXPORT BOOL
itmIsTime1EqualToTime2(ITMTIME *ptime1, ITMTIME *ptime2)
{
    if (ptime1->dwHiTime == ptime2->dwHiTime && 
        ptime1->dwLoTime == ptime2->dwLoTime)
        return TRUE;
    else
        return FALSE;
}


/* 
 * Determine if ptime1 is greater than or equal to ptime2
 */
EMUAPIEXPORT BOOL
itmIsTime1GreaterThanOrEqualToTime2(ITMTIME *ptime1, ITMTIME *ptime2)
{
    if (ptime1->dwHiTime > ptime2->dwHiTime)
        return TRUE;

    if (ptime1->dwHiTime == ptime2->dwHiTime && 
        ptime1->dwLoTime >= ptime2->dwLoTime)
        return TRUE;

    return FALSE;
}


/****************************************************************************
 * Internal functions
 ****************************************************************************/

static void
_computeInvocationTime(stEvent *pevent, ITMTIME *ptimeNow)
{
    ITMTIME time;

    if (pevent->schedinfo.dwFlags & ITMSF_ABSOLUTE) {
        pevent->time.dwLoTime = pevent->schedinfo.dwNumerator;
        pevent->time.dwHiTime = pevent->schedinfo.dwDenominator;
    }
    else
    {
        DWORD dwNum   = pevent->schedinfo.dwNumerator;
        DWORD dwDenom = pevent->schedinfo.dwDenominator;

        DPRINTF(("_computeInvocationTime dwNum %d dwDenom %d, timeNow (%d, %d)",
            dwNum, dwDenom, ptimeNow->dwHiTime, ptimeNow->dwLoTime));

        /* Be careful about overflow */
        ASSERT(dwNum < 88000);

        time.dwLoTime = dwNum * BASE_TIMER_FREQ / dwDenom;
        time.dwHiTime = 0;

        /* As a rule, we would far rather be a little late than a little
         * early with the callback, since in many cases being a little early
         * can cause valid data to get overwritten.  */
        while (time.dwLoTime * dwDenom < dwNum * BASE_TIMER_FREQ)
            time.dwLoTime++;

        /* For slower sample rates, we need make sure that
         * the interrupt occurs at the end of a sample period
         * rather than at the beginning or in the middle. */
        if (dwDenom < BASE_TIMER_FREQ)
            time.dwLoTime += (BASE_TIMER_FREQ / dwDenom) + 1;

        pevent->time = itmAddTimes(&time, ptimeNow);
        DPRINTF(("_computeInvocationTime for event 0x%x : %ld, %ld",
                pevent, pevent->time.dwHiTime, pevent->time.dwLoTime));
    }
}


static void
_addToEventList(stEvent *pevent)
{
    ITMTIME timeNow;
    stITMState *pstate = pevent->pstate;

    itmGetCurrentTime(pstate->itmid, &timeNow);

    _computeInvocationTime(pevent, &timeNow);

    /* Put the event onto the callback list */
    IDISABLE();
    pevent->pNextCallbackEvent = NULL;
    pevent->pNextEvent = pstate->pEventList;
    pstate->pEventList = pevent;
    
    /* Update the timer if necessary: note that this must be done after
     * the event is queued, or the timer code won't start the interrupt
     * when necessary.  */
    if (itmIsTime1LessThanOrEqualToTime2(&pevent->time, &pstate->timeNextCallback)) {
        pstate->timeNextCallback = pevent->time;
        _updateTimer(pstate, &timeNow, FALSE);
    }
    IENABLE();
}


/* 
 * Remove an event from the master event list.
 */
static BOOL
_removeFromEventList(stEvent *pevent)
{
    stITMState *pstate = pevent->pstate;
    stEvent    *pCurrEvent;
    stEvent    *pPrevEvent;
    BOOL        bFound = FALSE;

    pCurrEvent = pstate->pEventList;
    pPrevEvent = NULL;
    while (pCurrEvent) {
        if (pCurrEvent == pevent) {
            bFound = TRUE;

            /* Need to remove it */
            if (pPrevEvent)
                pPrevEvent->pNextEvent = pCurrEvent->pNextEvent;
            else
                pstate->pEventList = pCurrEvent->pNextEvent;

            break;
        }
         
        pPrevEvent = pCurrEvent;
        pCurrEvent = pCurrEvent->pNextEvent;
    }

    return bFound;
}


/*
 * Remove an entry from the callback list and make sure that
 * everything is scheduled correctly.
 */
static EMUSTAT
_cancelEvent(stEvent *pevent)
{
    stITMState *pstate = pevent->pstate;
    stEvent    *pCurrEvent;
    stEvent    *pPrevEvent;
    BOOL        bFound;

    IDISABLE();

    /* Remove from the event list */
    bFound = _removeFromEventList(pevent);

    /* Remove from the callback list too */
    pCurrEvent = pstate->pCallbackEventList;
    pPrevEvent = NULL;
    while (pCurrEvent) {
        if (pCurrEvent == pevent) {
            bFound = TRUE;

            if (pPrevEvent)
                pPrevEvent->pNextCallbackEvent = pCurrEvent->pNextCallbackEvent;
            else
                pstate->pCallbackEventList = pCurrEvent->pNextCallbackEvent;

            break;
        } 

        pPrevEvent = pCurrEvent;
        pCurrEvent = pCurrEvent->pNextCallbackEvent;
    }

    if (itmIsTime1EqualToTime2(&pevent->time, &pstate->timeNextCallback)) 
    {
        ITMTIME time, timeNow;

        /* Find the next event to run */
        pCurrEvent = pstate->pEventList;
        time.dwLoTime = 0xFFFFFFFF;
        time.dwHiTime = 0xFFFFFFFF;
        while (pCurrEvent) {
            if (itmIsTime1LessThanOrEqualToTime2(&pCurrEvent->time, &time))
                time = pCurrEvent->time;

            pCurrEvent = pCurrEvent->pNextEvent;
        }

        pstate->timeNextCallback = time;
        itmGetCurrentTime(pstate->itmid, &timeNow);
        _updateTimer(pstate, &timeNow, TRUE);
    }

    IENABLE();

    _freeEvent(pstate, pevent);

    return SUCCESS;
}


/*
 * Update the time data structures.  This routine needs to be run
 * periodically (once every five seconds or so) to insure that the
 * clock register doesn't overflow without our noticing.
 */
static void
_timeUpdateCallback(ITMID itmid)
{
    stITMState *pstate = FIND_STATE(itmid);
    if (pstate != NULL) {
        /* The following is slightly sleazy.  We know that calls to
         * itmGetCurrentTime have a side effect of updating the master
         * time accumulator in the state variable.  Se we just call it.
         */
        ITMTIME time;
        itmGetCurrentTime(itmid, &time);

        /* Reschedule */
        osRescheduleTimeout(pstate->hTimeout, 5000);
    }
}


/*
 * _itimerISRCallback gets invoked at ISR time whenever the interval
 * timer interrupt is triggered.  It reads the current time and checks
 * to see if the next callback is ready to trigger.
 */
static BOOL
_itimerISRCallback(IPHANDLE iphandle, enIPType type, DWORD dwUser, unIPReply *preply)
{
    ITMTIME     timeNow;
    stITMState *pstate = (stITMState*) dwUser;
    BOOL        bRunAtCallbackTime = FALSE;

    /* Figure out what time it is */
    itmGetCurrentTime(pstate->itmid, &timeNow);

    /* Check to see if we've reached the time of the next event */
    if (itmIsTime1GreaterThanOrEqualToTime2(&timeNow, &pstate->timeNextCallback)) {
        stEvent    *pCurrEvent = pstate->pEventList;
        stEvent    *pNextEvent = NULL;

        /* We shouldn't be interrupting if there are no events scheduled */
        ASSERT(pCurrEvent);

        while (pCurrEvent) {

            /* We're bouncing this event structure all over the place,
             * so we need to lasso the next event structure early.  */
            pNextEvent = pCurrEvent->pNextEvent;

            /* Check to see whether this callback should be invoked */
            if (itmIsTime1GreaterThanOrEqualToTime2(&timeNow, &pCurrEvent->time))
            {
                /* Check to see whether we should run the callback now */
                if (pCurrEvent->schedinfo.dwFlags & ITMSF_RUN_AT_ISR_TIME) 
                {
                    /* This is a high-priority event, so run it now. */
                    if (!pCurrEvent->schedinfo.fCallback(&pCurrEvent->schedinfo))
                    {
                        /* Callback indicated that it didn't want to be
                         * rescheduled, so take it off the event list */
                        _removeFromEventList(pCurrEvent);

                        /* Stick it onto the free list; note that this
                         * alters the next event pointer, which is why
                         * we saved it earlier.  */
                        pCurrEvent->pNextEvent = pstate->pFreeEventList;
                        pstate->pFreeEventList = pCurrEvent;
                    } else {
                        /* Recompute the time at which this event should run
                         * next. */
                        itmGetCurrentTime(pstate->itmid, &timeNow);
                        _computeInvocationTime(pCurrEvent, &timeNow);
                    }
                } 
                else 
                {
                    /* This is a low-priority callback; increment the
                     * invocation count and make sure that it is on the
                     * callback event list.  */
                    pCurrEvent->dwInvocationCount++;
                    if (!pCurrEvent->bOnCallbackList) {
                        bRunAtCallbackTime = TRUE;
                        pCurrEvent->pNextCallbackEvent = pstate->pCallbackEventList;
                        pCurrEvent->bOnCallbackList = TRUE;
                        pstate->pCallbackEventList = pCurrEvent;
                    }

                    /* If this is an absolute event, or if we're not counting
                     * callbacks, set its trigger time to infinite so that it
                     * won't run again until we get around to calling the 
                     * low-level callback.  */
                    if ((pCurrEvent->schedinfo.dwFlags & ITMSF_ABSOLUTE) ||
                        !(pCurrEvent->schedinfo.dwFlags & ITMSF_COUNT_CALLBACKS))
                    {
                        pCurrEvent->time.dwLoTime = 0xFFFFFFFF;
                        pCurrEvent->time.dwHiTime = 0xFFFFFFFF;
                    }
                    else
                    {
                        /* For callbacks with callback counting enabled, we
                         * need to reschedule it now event though we don't know
                         * for sure whether the callback routine will return TRUE,
                         * otherwise we could miss an invocation if it takes a long
                         * for the callback routine to run.
                         */
                        itmGetCurrentTime(pstate->itmid, &timeNow);
                        _computeInvocationTime(pCurrEvent, &timeNow);

                    }
                }
            }

            /* Move on to the next event */
            pCurrEvent = pNextEvent;

            /* Don't let the time stray if we end up running long routines */
            itmGetCurrentTime(pstate->itmid, &timeNow);

        } /* end while */

        _updateTimer(pstate, &timeNow, TRUE);
    }
     
    return bRunAtCallbackTime;
}


/* 
 * This function gets invoked at callback time.  It sweeps through
 * the event list, looking for events which have a non-zero 
 * callback invocation count.  Since it is possible that the ISR associated
 * with a particular event may have run many times before the callback level
 * code got a chance to run, we have to keep track of how many times we
 * need to invoke the callback.
 *
 * Note that, unlike the ISR code, the callback code does not reuse.
 */
static BOOL
_itimerCallback(IPHANDLE iphandle, enIPType type, DWORD dwUser, 
                unIPReply *preply)
{
    stITMState *pstate = (stITMState*) dwUser;
    stEvent    *pCurrEvent;
    stEvent    *pNextEvent;
    BOOL        bReschedule = TRUE;
    DWORD       i;

    /* Pull all of the events off the callback list atomically */
    IDISABLE();
    pCurrEvent = pstate->pCallbackEventList;
    pstate->pCallbackEventList = NULL;
    IENABLE();

    while (pCurrEvent) {

#ifdef DEBUG
        {
            ITMTIME timeNow;
            itmGetCurrentTime(pstate->itmid, &timeNow);
        }
#endif

		ASSERT(pCurrEvent->dwInvocationCount != 0);

        if (!(pCurrEvent->schedinfo.dwFlags & ITMSF_COUNT_CALLBACKS)) {
            pCurrEvent->dwInvocationCount = 1;
        }
#ifdef DEBUG
        else {
        }
#endif

		for (i = 0; i < pCurrEvent->dwInvocationCount; i++) {
            ITMTIME timeNow;
            itmGetCurrentTime(pstate->itmid, &timeNow);
            if (!pCurrEvent->schedinfo.fCallback(&pCurrEvent->schedinfo)) 
            {
				bReschedule = FALSE;
                break;
            }
		}

        pCurrEvent->dwInvocationCount = 0;
        pCurrEvent->bOnCallbackList = FALSE;

        /* Advance to the next event on the callback list */
		pNextEvent = pCurrEvent->pNextCallbackEvent;
		pCurrEvent->pNextCallbackEvent = NULL;


        if (bReschedule) {
            /* Recompute invocation time */
            ITMTIME timeNow;
            itmGetCurrentTime(pstate->itmid, &timeNow);
            _computeInvocationTime(pCurrEvent, &timeNow);
            _updateTimer(pstate, &timeNow, TRUE);
        } else {
            /* Remove from the event list */
            IDISABLE();
            _removeFromEventList(pCurrEvent); 
            IENABLE();
		}

        pCurrEvent = pNextEvent;
    }

    return TRUE;
}


/*
 * Examine the event list and update the timer appropriately.
 */
static void
_updateTimer(stITMState *pstate, ITMTIME *ptimeNow, BOOL bScanList)
{
    stIPInfo    ipInfo;
#ifdef USE_VAR_TIMER
    ITMTIME timeDifference;
#endif

    /* Check to see whether we can turn off the interrupt
     * handler.
     */
    if (pstate->pEventList == NULL && 
        pstate->pCallbackEventList == NULL) {
        /* No events are ready to be queued, so just turn off the
         * interrupt.
         */
        ipUnregisterCallback(pstate->iphandle);
        pstate->iphandle = 0;
        pstate->timeNextCallback.dwLoTime = 0xFFFFFFFF;
        pstate->timeNextCallback.dwHiTime = 0xFFFFFFFF;
        return;
    }

    /* If bScanList is true, we need to search the list for
     * the next callback time and update timeNextCallback here.
     * Otherwise, we assume that it has already been done for 
     * us.
     */
    if (bScanList) {
        ITMTIME  timeSmall;
        stEvent *pCurrEvent;

        timeSmall.dwLoTime = 0xFFFFFFFF;
        timeSmall.dwHiTime = 0xFFFFFFFF;

        pCurrEvent = pstate->pEventList;
        while (pCurrEvent) {
            if (itmIsTime1LessThanTime2(&pCurrEvent->time, &timeSmall))
                timeSmall = pCurrEvent->time;

            pCurrEvent = pCurrEvent->pNextEvent;
        }

        pstate->timeNextCallback = timeSmall;
    }
                
#ifdef USE_VAR_TIMER
    /* Program the timer */
    if (itmIsTime1LessThanOrEqualToTime2(ptimeNow, &pstate->timeNextCallback)) {
        timeDifference = itmSubtractTimes(&pstate->timeNextCallback, ptimeNow);
        if (timeDifference.dwHiTime || timeDifference.dwLoTime > 1023) 
            timeDifference.dwLoTime = 0;
        /* We don't want to set the interval timer to a value less than 5. */
        else if (timeDifference.dwLoTime < MIN_TIMER_VALUE)
            timeDifference.dwLoTime = MIN_TIMER_VALUE;

    } else {
        /* Hmm, the time to be scheduled is in the past.  Pick it up
         * next time. This could happen if some callback takes a long time. */
        timeDifference.dwLoTime = MIN_TIMER_VALUE;
    }
#endif

    /* Otherwise, we need to do some work.  First, we make sure that
     * our interrupt handlers are registered.  */
    if (pstate->iphandle == 0) {
        ipInfo.type = IP_INTTIMER;
#ifdef USE_VAR_TIMER
//        ipInfo.interruptParameter  = timeDifference.dwLoTime;
        ipInfo.interruptParameter  = 0;
#else
        ipInfo.interruptParameter  = TIMER_FREQUENCY;
#endif
        ipInfo.userParameter       = (DWORD) pstate;
        ipInfo.fISRHandler         = _itimerISRCallback;
        ipInfo.fHandler            = _itimerCallback;
        if (ipRegisterCallback(pstate->ipid, &ipInfo, &pstate->iphandle) != SUCCESS) {
            ASSERT(0);
        }
    }

#ifdef USE_VAR_TIMER
    if (timeDifference.dwLoTime != 0)
    {
        /* Recompute the current time so we get a more accurate first interrupt */
        ITMTIME newTimeNow;
        ITMTIME newTimeDifference;

        itmGetCurrentTime(pstate->itmid, &newTimeNow);
        newTimeDifference = itmSubtractTimes(&newTimeNow, ptimeNow);
        ASSERT(newTimeDifference.dwHiTime == 0);
        if (timeDifference.dwLoTime > newTimeDifference.dwLoTime + MIN_TIMER_VALUE)
            timeDifference.dwLoTime -= newTimeDifference.dwLoTime;
        else
            timeDifference.dwLoTime = MIN_TIMER_VALUE;
        W8010SERegWrite(pstate->halid, TIMR, (WORD)timeDifference.dwLoTime);
    }
#endif
}


/* 
 * Allocate a new event structure.  The free list is first searched
 * before calling heap alloc.
 */
static stEvent *
_allocEvent(stITMState *pstate)
{
    if (pstate->pFreeEventList) {
        stEvent *pNewEvent = pstate->pFreeEventList;
        pstate->pFreeEventList = pNewEvent->pNextEvent;
        return pNewEvent;
    } else
        return (stEvent *)osHeapAlloc(sizeof(stEvent), 0);
}


static void
_freeEvent(stITMState *pstate, stEvent *pcbi)
{
    stEvent *pCurrEvent;
    
    /* Atomically dequeue all of the events from the
     * free list */
    IDISABLE();
    pCurrEvent = pstate->pFreeEventList;
    pstate->pFreeEventList = NULL;
    IENABLE();

    /* Free anything on the free list */
    while (pCurrEvent) {
        stEvent *pNextEvent = pCurrEvent->pNextEvent;
        pCurrEvent->dwMagic = 0;
        osHeapFree(pCurrEvent);
        pCurrEvent = pNextEvent;
    }
    pstate->pFreeEventList = NULL;

    pcbi->dwMagic = 0;
    osHeapFree(pcbi);
}

