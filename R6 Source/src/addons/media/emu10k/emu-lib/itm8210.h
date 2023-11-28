/*****************************************************************************
*
*                             Copyright (c) 1998
*                E-mu Systems Proprietary All rights Reserved
*
*****************************************************************************/

/*****************************************************************************
*
* @doc INTERNAL
* @module itm8210.h | 
* This file contains the public datatypes and functions used by the EMU 8010
* interval timer manager.
*
* @iex 
* Revision History:
*
* Person            Date			Reason
* ---------------	------------    --------------------------------------
* John Kraft		July 13, 1998	Created.
*
*****************************************************************************
* @doc EXTERNAL 
*  This document describes the ITM8210 Interval Timer Manager API.  The ITM
*  virtualizes the 8010's on-chip interval timer and allows multiple clients
*  to schedule callbacks at precise intervals simultaneously.  It is used
*  extensively by other facilities to set up timeouts, run periodic 
*  housekeeping operations, and other such tasks.
*  
*  
* @contents1 Contents |
* 
* @subindex Enumerations
* @subindex Typedefs & Structures
* @subindex Public Functions
******************************************************************************/
#ifndef __ITM8210_H
#define __ITM8210_H

/****************
 * Include files
 ****************/

#include "datatype.h"
#include "aset8210.h"


/******************************************************************************
* @contents2 Enumerations And Public Definitions |
* @index enum |
******************************************************************************/

/* Error values */
#define ITMERR_BAD_ID           1
#define ITMERR_BAD_HANDLE       2
#define ITMERR_BAD_PARAM        3
#define ITMERR_NO_MEMORY        4
#define ITMERR_INIT_FAILED      5
#define ITMERR_INVALID_TIME     6


/* Flags for itmScheduleCallback */
#define ITMSF_ABSOLUTE              0x1
#define ITMSF_COUNT_CALLBACKS       0x2
#define ITMSF_RUN_AT_ISR_TIME       0x4

/***************************************************************************
 * @contents2 Types and structure definitions |
 * @index type,struct |
 ***************************************************************************/

/* @type ITMID |
 *  An opaque handle which is used to reference a particular Interval
 *  Timer Manager.  This value is returned by <f itmDiscoverChip>.
 */
typedef DWORD ITMID;


/* @type ITMHANDLE |
 *  An opaque handle that references a particular scheduled callback.
 *  This handle is initialized by one of the scheduling routines 
 *  (<f itmScheduleCallback> and <f itmSchedulePeriodicCallback>).
 */
typedef DWORD ITMHANDLE;


/* @type ITMCALLBACK |
 *  Defines the type of the callback function provided by the user.
 *  If the callback returns TRUE, a new callback will be scheduled
 *  using the parameter values in the schedinfo structure passed.
 *  The user is free to alter any of the fields in pSchedInfo, so
 *  if the user wishes to alter the callback period he or she can do
 *  so simply by changing pSchedInfo->dwNumerator or pSchedInfo->dwDenominator.
 *  If the user returns FALSE, the callback will not be rescheduled.
 *
 *  The first parameter, pSchedInfo, points to a copy of the ITMSCHEDINFO
 *  with which the callback was originally scheduled. 
 *
 */
typedef BOOL (*ITMCALLBACK)(struct stItmSchedInfo *pSchedInfo);


/* @type ITMTIME | 
 *  An ITMTIME contains a single time value, broken into two 32-bit
 *  halves.  We pass this structure around because we don't currently
 *  have widespread support for 64-bit integers.
 */
typedef struct {
    DWORD dwLoTime;
    DWORD dwHiTime;
} ITMTIME;


/* @type ITMSCHEDINFO |
 *  This type defines all of the information needed when scheduling
 *  a callback.  Callbacks are scheduled to occur at a precise point
 *  in time indicated by the dwNumerator and dwDenominator fields in
 *  this structure.  If dwNumerator is set to 10 and dwDenominator is
 *  set to 44100, the callback will be invoked in 10/44100's of a second.
 *  The ability to specify both the numerator and the denominator makes
 *  it easy to specify things in terms of sample periods.  
 */
typedef struct stItmSchedInfo {
    DWORD       dwNumerator;    /* @field The numerator in the callback time
                                 *  fraction (see above).  */
    DWORD       dwDenominator;  /* @field The denominator of the callback time 
                                 *  fraction.  */
    DWORD       dwFlags;        /* @field Scheduling control flags.  Currently
                                 *  the following values are supported:
                                 *  ITMSF_ABSOLUTE indicates that dwNumerator
                                 *  and dwDenominator should be treated as 
                                 *  an absolute time where dwNumerator contains
                                 *  the low-order bits of the ITMTIME and
                                 *  dwDenominator contains the high-order bits.
                                 *  ITMSF_COUNT_CALLBACKS insures that the
                                 *  callback routine is called once for every
                                 *  occurrence of the ISR.  If not specified,
                                 *  the first occurence of the ISR will
                                 *  schedule a callback, but if multiple ISRS
                                 *  occur before the callback gets invoked 
                                 *  then it will still only get invoked once.
                                 *  ITMSF_RUN_AT_ISR_TIME will cause the 
                                 *  function to run at high 
                                 *  priority at ISR time, rather than at the
                                 *  usual low-level priority.  Note that 
                                 *  ISR callbacks are severly restricted in
                                 *  what they can do.
                                 */
    ITMCALLBACK fCallback;      /* @field the address of the callback routine */
    DWORD       dwUser1;        /* @field Free for use by the user. */
    DWORD       dwUser2;        /* @field Free for use by the user. */
    DWORD       dwUser3;        /* @field Free for user by the user. */
} ITMSCHEDINFO;

/***************************************************************************
 * @contents2 Public Functions |
 * @index func |
 ***************************************************************************/

BEGINEMUCTYPE

/* @func Schedules a callback to execute.  This routine can be used to
 *  schedule both periodic and non-periodic callbacks by setting the
 *  dwPeriod field of the ITMSCHEDINFO structure appropriately.  
 *
 * @parm ITMID | id | The ID of the interval timer manager with whom the
 *  callback should be scheduled.
 * @parm ITMSCHEDINFO * | pschedinfo | A pointer to the scheduling information
 *  for the callback.
 * @parm ITMHANDLE * | pitmhandle | A pointer to the memory which will receive
 *  the handle of the new callback.
 *
 * @rdesc Returns SUCCESS if the callback is successfully scheduled.
 *  Otherwise, one of the following values is returned:
 *  @flag ITMERR_BAD_ID | The ITMID given is invalid.
 *  @flag ITMERR_BAD_PARAM | The ITMSCHEDINFO pointer is invalid.
 *  @flag ITMERR_BAD_HANDLE | The ITMHANDLE pointer is invalid.
 *  @flag ITMERR_NO_MEMORY | An internal memory allocation failed.
 */
EMUAPIEXPORT EMUSTAT itmScheduleCallback(ITMID, ITMSCHEDINFO*, ITMHANDLE *retHandle);

/* @func Cancel a previously scheduled callback.  A callback may be cancelled
 *  at any time, 
 * 
 * @parm ITMHANDLE | itmh | The handle of the callback to be cancelled.
 *
 * @rdesc Returns SUCCESS if the callback is successfully cancelled.
 *  Otherwise, one of the following error codes is returned:
 *  @flag ITMERR_BAD_HANDLE | The handle passed was either invalid or 
 *   has already been cancelled.
 */
EMUAPIEXPORT EMUSTAT itmCancelCallback(ITMHANDLE);


/* @func Retrieve a structure which contains the current time.  
 * 
 * @parm ITMID | id | The ID of the interval timer manager to read from.
 * @parm ITMTIME * | pitmtime | A pointer to the ITMTIME structure which
 *  will receive the current time.  This time is in units of 20.833 microseconds
 *  (1/48000th of a second).
 *
 * @rdesc Returns SUCCESS if ITMTIME is successfully filled. Otherwise:
 *  @flag ITMERR_BAD_ID | The ITM ID is invalid.
 *  @flag ITMERR_BAD_PARAM | The ITMTIME pointer is invalid.
 */
EMUAPIEXPORT EMUSTAT itmGetCurrentTime(ITMID, ITMTIME *);


/* @func Retrieve a time structure describing when the timeout will trigger
 * @parm ITMHANDLE | itmh | The handle of the timeout event.
 * @parm ITMTIME * | ptime | A pointer to an stTime structure to be filled.
 *
 * @rdesc Returns SUCCESS if ITMTIME is successfully filled.  Otherwise:
 *  @flag ITMERR_BAD_HANDLE | The ITMHANDLE is bogus.
 *  @flag ITMERR_BAD_PARAM  | The ITMTIME pointer is invalid.
 */
EMUAPIEXPORT EMUSTAT itmGetTriggerTime(ITMHANDLE, ITMTIME*);


/* @func Change the scheduling information for an event.
 * @parm ITMHANDLE | itmh | The handle of the timeout event whose information
 *  is to be changed.
 * @parm ITMSCHEDINFO * | psi | The new scheduling info.
 */
EMUAPIEXPORT EMUSTAT itmChangeSchedInfo(ITMHANDLE, ITMSCHEDINFO*);

/* @func Time arithmetic routines.
 */
EMUAPIEXPORT ITMTIME itmAddTimes(ITMTIME*, ITMTIME*);
EMUAPIEXPORT ITMTIME itmSubtractTimes(ITMTIME*, ITMTIME*);
EMUAPIEXPORT BOOL    itmIsTime1LessThanOrEqualToTime2(ITMTIME*, ITMTIME*);
EMUAPIEXPORT BOOL    itmIsTime1EqualToTime2(ITMTIME*, ITMTIME*);
EMUAPIEXPORT BOOL    itmIsTime1GreaterThanOrEqualToTime2(ITMTIME*, ITMTIME*);
#define itmIsTime1GreaterThanTime2(_x, _y) \
    !itmIsTime1LessThanOrEqualToTime2(_x, _y)
#define itmIsTime1LessThanTime2(_x, _y) \
    !itmIsTime1GreaterThanOrEqualToTime2(_x, _y)

ENDEMUCTYPE

#endif /* __ITM8210_H */

