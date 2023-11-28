/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*
* @doc DISCOVER
* @module se8210d.h | 
*   Contains the chip discovery interface for the se8210 module.  This
*   interface is semi-private and is only exposed to the hardware resource
*   manager.
*
* @iex
*  Version	Person	Date		Reason
*  -------	------	----		---------------------------------- 
*  0.001	JK	Jun 02, 97	Initial version
* @end
*
******************************************************************************
*/

#ifndef __SE8210D_H
#define __SE8210D_H

/***************
 * Includes
 ***************/

#include "datatype.h"
#include "hal8210.h"
#include "cfg8210.h"
#include "se8210.h"
#include "itm8210.h"


/* @struct SECHIPCONFIG | The configuration data structure */
typedef struct stSEConfigTag
{
	BOOL  bInitSoundEngine;    /* @field Indicates that we should initialize
	                            *  the sound engine when discovered. */
   ITMID  itmid;              /* @field Interval Timer Manager ID */
   DWORD dwHWRevision;         /* @field The current ASIC revision */
} stE8010SEConfig, SECHIPCONFIG;


/**************
 * Functions 
 **************/

/* @func Initialize the sound engine module's internal data structures.
 *	This routine must be called once (and only once) when the audio system
 *  is first being initialized.
 *
 * @rdesc Returns SUCCESS if initialization succeeded, and EMUCHIP_INIT_FAILED
 *  otherwise.
 */
EMUAPIEXPORT EMUSTAT seInit(void);

/* @func Initialize the sound engine on the specified EMU8010 to a known
 *  quiescent state.  If the initialization completes successfully, all
 *  voices will be silent and the sound engine registers will be in a 
 *  consistent state.  
 *
 * @parm HALID | haldID | Contains the Hardware Abstraction Layer ID of the 
 *	E8010 chip whose sound engine is to be discovered.
 * @parm SECONFIG * | config | A pointer to a configuration description.
 * @parm SEID * | rid | A pointer to memory for the returned E8010ID.  
 *  Upon successful completion, <f seDiscoverChip> will write the ID of the discovered sound
 *  engine into the memory at this address.
 *
 * @rdesc Returns SUCCESS if the discovery procedure completes successfully.
 *  Otherwise, one of the following values will be returned:
 *		@flag SERR_BAD_HANDLE   | No E8010 with the given halID was found.
 *		@flag SERR_INIT_FAILED | Could not successfully initialize the sound engine.
 */
EMUAPIEXPORT EMUSTAT seDiscoverChip(HALID halID, SECHIPCONFIG *config, 
									SEID *rid /* IO */);

/* @func Unitialize the sound engine portion of the chip and release any
 *  allocated storage.  
 *
 * @parm SEID | seid | The ID of the sound engine whose resources we want
 *  to release.
 * 
 * @rdesc Upon successful completion, this routine returns SUCCESS.  On failure,
 *  one of the following values is returned:
 *		@flag SERR_BAD_HANDLE | The given SEID is invalid.
 */
EMUAPIEXPORT EMUSTAT seUndiscoverChip(SEID seid);

#endif /* __SE8210D_H */
