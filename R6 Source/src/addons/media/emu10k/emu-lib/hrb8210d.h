/*****************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
*****************************************************************************/

/*****************************************************************************
*
* @doc INTERNAL
* @module hrb8210d.h | 
* This file contains the data types and functions used in the process of
* system initialization and chip discovery.  Basically, no one API other than
* hrmInit() and hrmDiscoverChip() should concern itself with this stuff.
*
* @iex 
* Revision History:
*
* Person            Date			Reason
* ---------------	------------    --------------------------------------
* John Kraft		Oct 27, 97		Created.
*
******************************************************************************/
#ifndef __HRB8210D_H
#define __HRB8210D_H

/****************
 * Include files
 ****************/

#include "datatype.h"
#include "hal8210.h"
#include "hrm8210.h"
#include "cfg8210.h"
#include "hrb8210.h"


/* @struct HRBCONFIG | The configuration data structure */
typedef struct  {
	HRMCONTIGMEMORY		hbSrcRecord;	/* @field Contiguous buffer for ADC */
	HRMCONTIGMEMORY		hbMicRecord;    /* @field Buffer for Microphone */
	HRMCONTIGMEMORY		hbFXRecord;     /* @field Buffer for FX output */
} HRBCONFIG;


/**************
 * Functions 
 **************/

BEGINEMUCTYPE

/* @func Initialize the HRB module's internal data structures.
 *	This routine must be called once (and only once) when the audio system
 *  is first being initialized.
 *
 * @rdesc Returns SUCCESS if initialization succeeded, and EMUCHIP_INIT_FAILED
 *  otherwise.
 */
EMUAPIEXPORT EMUSTAT hrbInit(void);

/* @func Initialize the hardware recording buffer manager on the specified 
 *  EMU8010 to a known quiescent state. 
 *
 * @parm HALID | haldID | Contains the Hardware Abstraction Layer ID of the 
 *	E8010 chip whose HRB is to be discovered.
 * @parm HRMCONFIG * | config | A pointer to a configuration description.
 * @parm HRMID * | rid | A pointer to memory for the returned E8010ID.  
 *  Upon successful completion, <f hrbDiscoverChip> will write the ID of 
 *  the discovered HRB into the memory at this address.
 *
 * @rdesc Returns SUCCESS if the discovery procedure completes successfully.
 *  Otherwise, one of the following values will be returned:
 *		@flag HRBERR_BAD_HANDLE  | No E8010 with the given halID was found.
 *		@flag HRBERR_INIT_FAILED | Could not successfully initialize the HRB.
 */
EMUAPIEXPORT EMUSTAT hrbDiscoverChip(HALID halid, IPID ipid, DWORD itmid, HRBCONFIG *config, 
									 HRBID *rid /* IO */);

/* @func Unitialize the recording portion of the chip and release any
 *  allocated storage.  
 *
 * @parm HRBID | seid | The ID of the HRB whose resources we want
 *  to release.
 * 
 * @rdesc Upon successful completion, this routine returns SUCCESS.  On failure,
 *  one of the following values is returned:
 *		@flag HRBERR_BAD_HANDLE | The given HRBID is invalid.
 */
EMUAPIEXPORT EMUSTAT hrbUndiscoverChip(HRBID hrbid);

ENDEMUCTYPE

#endif /* __HRB8210D_H */
