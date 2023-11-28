
/*********************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* @doc INTERNAL
* @module hrm8010d.h |
* EMU8010 hardware resource manager discovery functions 
* 
* @iex
* Revision History:
*
* Version    Person        Date         Reason
* -------    ---------   -----------  --------------------------------------- 
*  0.003        MG		 Oct 09, 97	  Auto-ducked
*  0.002		JK		 Sep 24, 97	  Changed to use the 8210 name scheme.
*  0.001        MG       Jun 02, 97   Initial version
*
******************************************************************************
* @doc EXTERNAL
* @contents1 EMU HRM8210 Programmer's Manual |
* These are the series of routines needed for system configuration and debug.
*/

#ifndef __HRM8210D_H
#define __HRM8210D_H

/************
* Includes
************/

#include "datatype.h" /* Data types */  
#include "cfg8210.h"  /* Calling convention, config structure */
#include "hrm8210.h"

/****************
 * Definitions
 ****************/

 /****************
 * Structures
 ***************/

BEGINEMUCTYPE

/* NOTE: These functions exposed to hardware initialization ONLY */

/* @func Initialize all components of the EMU8210 Hardware Resource Manager
 *	This routine must be called once (and only once) when the audio system
 *  is first being initialized.
 *
 * @rdesc Returns SUCCESS if initialization succeeded, and EMUCHIP_INIT_FAILED
 *  otherwise.
 */
EMUAPIEXPORT EMUSTAT  hrmInit(void);

/* @func Initialize all components on the specified PCI function of an EMU8010 to a known
 *  quiescent state.     
 *
 * @parm HRMID * | *stuffedWithID | Will be stuffed with a unique ID of the 
 *	EMU8010 chip whose resources are discovered. 
 * @parm HRMCHIPCONFIG * | config | A pointer to a configuration description. See cfg8210.h
 *
 * @rdesc Returns SUCCESS if the discovery procedure completes in all modules successfully. 
 *
 */
EMUAPIEXPORT EMUSTAT  hrmDiscoverChip(HRMID *id /* IO */, HRMCHIPCONFIG *config);

/* @func Unitialize all components on the specified PCI function of an EMU8010 chip
 *  and release any allocated storage.  
 *
 * @parm HRMID | hrmID | The ID returned by hrmDiscoverChip 
 * @parm HRMFLAGS | unDiscoverFlags | Indicates which PCI functions have been un-discovered. 
 * See configuration structure in'cfg8210.h'
 * 
 * @rdesc Upon successful completion, this routine returns SUCCESS.   
 */
EMUAPIEXPORT EMUSTAT  hrmUndiscoverChip(HRMID id, HRMFLAGS hrmUndiscoverFlags);

/* @func For a given HRM8210 ID, return the unique user supplied Hardware ID  
 *
 * @parm HRMID | hrmID | 'ID' value returned by halDiscoverChip 
 *
 * @rdesc Return of 0xFFFFFFFF means the HALID was invalid.
 */
EMUAPIEXPORT DWORD    hrmGetUserHardwareID(HRMID id);

/* @func For a given HRM8210 ID, return whether one or more PCI functions have been
 * discovered.
 *
 * @parm HRMID | hrmID | 'ID' value returned by halDiscoverChip
 * @parm HRMFLAGS | unDiscoverFlags | Indicates which PCI function(s) are in question. 
 * See configuration structure in'cfg8210.h'
 *
 * @rdesc Returns TRUE if all PCI functions corresponding to the user set flags were 
 * discovered, FALSE otherwise.
 */
EMUAPIEXPORT BOOL     hrmIsDiscovered(HRMID id, HRMFLAGS hrmIsInit);

/* NOTE: This function exposed for debug ONLY! */

/* @func For a given HRM8210 ID, return the HALID.
 *
 * @parm HRMID | hrmID | 'ID' value returned by halDiscoverChip 
 *
 * @rdesc Returns the HALID
 */
EMUAPIEXPORT DWORD   hrmGetHALID(HRMID id);

ENDEMUCTYPE

#endif
