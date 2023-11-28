/****************************************************************************\
 *  File:	itm8210d.h														*
 *  Data:	July 13, 1998													*
 *																			*
 *  Contains the discovery functions for the Interval Timer Manager.        *
 *                                                                          *
 ****************************************************************************
 *  Copyright (C) 1998 E-mu Systems Inc.  All rights reserved.              *
\****************************************************************************/

#ifndef __ITM8210D_H
#define __ITM8210D_H

/****************
 * Include files
 ****************/

#include "datatype.h"
#include "hal8210.h"
#include "cfg8210.h"
#include "ip8210.h"
#include "itm8210.h"

/**************
 * Functions 
 **************/

BEGINEMUCTYPE

/* @func Initialize the ITM module's internal data structures.
 *	This routine must be called once (and only once) when the audio system
 *  is first being initialized.
 *
 * @rdesc Returns SUCCESS if initialization succeeded, and EMUCHIP_INIT_FAILED
 *  otherwise.
 */
EMUAPIEXPORT EMUSTAT itmInit(void);


/* @func Initialize the Interval Timer Manager to a known quiescent state. 
 *
 * @parm HALID | haldID | Contains the Hardware Abstraction Layer ID of the 
 *	E8010 chip whose ITM is to be discovered.
 * @parm IPID  | ipid   | Contains an ID for the associated Interrupt Pending Manager.
 * @parm ITMID * | rid | A pointer to memory for the returned ITMID.  
 *  Upon successful completion, <f itmDiscoverChip> will write the ID of 
 *  the discovered ITM into the memory at this address.
 *
 * @rdesc Returns SUCCESS if the discovery procedure completes successfully.
 *  Otherwise, one of the following values will be returned:
 *		@flag ITMERR_BAD_HANDLE  | No E8010 with the given halID was found.
 *		@flag ITMERR_INIT_FAILED | Could not successfully initialize the ITM.
 */
EMUAPIEXPORT EMUSTAT itmDiscoverChip(HALID halid, IPID ipid, ITMID *retID);

/* @func Unitialize the timer manager of the chip and release any
 *  allocated storage.  
 *
 * @parm ITMID | itmid | The ID of the ITM whose resources we want
 *  to release.
 * 
 * @rdesc Upon successful completion, this routine returns SUCCESS.  On failure,
 *  one of the following values is returned:
 *		@flag ITMERR_BAD_HANDLE | The given HRBID is invalid.
 */
EMUAPIEXPORT EMUSTAT itmUndiscoverChip(ITMID hrbid);

ENDEMUCTYPE


#endif /* __ITM8210D_H */
