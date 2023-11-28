
/*********************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* @doc INTERNAL
* @module mxr8010d.h |
*  Discovery routines for the mixer.  This 
*
* @iex
* Revision History:
*
*  Version  Person	Date		Reason
*  -------  ------	----------	--------------------------------------- 
*  0.001	JK		Oct 14, 97	Created.

******************************************************************************
*/

#ifndef __MXR8210D_H
#define __MXR8210D_H

/************
* Includes
************/

#include "datatype.h" /* Data types */  
#include "hal8210.h"
#include "mxr8210.h"


/************
 * Definitions 
 ************/

#define MXRID_AC97  0x0
#define MXRID_ECARD	0xE

/************
 * Structures 
 ************/

/* @struct MXRCHIPCONFIG | 
 *  The configuration data structure.
 */
typedef struct stMixer8210Config
{
	DWORD mixerID;    /* @field A unique 32 bit value known by the client for a 
				       * particular mixer configuration. */
    BOOL  bInitMixer; /* @field Boolean indicating whether we should initialize
					   *  the mixer (or not, if == 0)  */
} MXRCONFIG;



/****************
 * Functions 
 ****************/

/* @func Initialize all components of the MXR8210
 *	This routine must be called once (and only once) when the audio system
 *  is first being initialized.
 * 
 * @rdesc Returns SUCCESS if initialization succeeded, and EMUCHIP_INIT_FAILED
 *  otherwise.
 */
EMUAPIEXPORT EMUSTAT  mxrInit();

/* @func Initialize all components of the external mixer to a known quiescent state,
 * and enable audio passthrough.     
 *
 * @parm DWORD | halid | The HALID through which hardware is programmed
 * @parm MXRCHIPCONFIG * | config | A pointer to a configuration description.
 * @parm MXRID * | *stuffedWithID | Will be stuffed with a unique ID of the 
 *	EMU8010 chip whose resources are discovered. 
 *
 * @rdesc Returns SUCCESS if the discovery procedure completes in all modules successfully.
 */
EMUAPIEXPORT EMUSTAT  mxrDiscoverChip(HALID, MXRCONFIG *config, MXRID *id /* IO */);

/* @func Unitialize all components of the mixer.  
 *
 * @parm MXRID | hrmID | The ID returned by mxrDiscoverChip 
 * 
 * @rdesc Upon successful completion, this routine returns SUCCESS.   
 */
EMUAPIEXPORT EMUSTAT  mxrUndiscoverChip(MXRID id);

#endif /* __MX8210D_H */
