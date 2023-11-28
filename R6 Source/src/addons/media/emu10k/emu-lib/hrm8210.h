
/*********************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*/

/******************************************************************************
*
* @doc INTERNAL
* @module hrm8010.h |
* EMU8010 hardware resource manager utility functions 
* 
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
* HRM8210 is the main interface to all hardware resource manager modules. 
* The EMU8010 many components, but only some of those components are actually 
* relevant at any given time. Also, there are times when a single basic function 
* (such as initialization and chip discovery) needs to be applied to all 
* modules, and typically that will happen at the same time. Such functions are 
* handled by a single blanket function call. Also, the EMU8010 has certain one-shot 
* type features which fall under no particular component category. Finally, there
* needs to be a module which can correlate an instance of hardware between modules
* (IE what is the sound engine that is tied to this effects engine?) This object 
* serves these types of services.
*/

#ifndef __HRM8210_H
#define __HRM8210_H

/************
* Includes
************/

#include "datatype.h" /* Data types */
#include "aset8210.h"

/****************
 * Definitions
 ****************/

 /****************
 * Structures
 ***************/
BEGINEMUCTYPE

/* @type HRMID | An opague handle used to reference an instance of hardware.
 */
typedef DWORD HRMID; 

/* @msg HRM_INVALID_ID | Macro indicating a bad hardware ID 
 */
#define HRM_INVALID_ID 0

/* @msg HRM_IS_INVALID_ID | Macro returning TRUE if a bad hardware ID, FALSE otherwise 
 */
#define HRM_IS_INVALID_ID(a) (a==HRM_INVALID_ID)

/* Public functions */

/* @func This function fills an array with the IDs of all of
 *  the discovered EMU8010s in the system and returns a count of the
 *  total number of EMU8010s which have been discovered or partially discovered.  
 *  The caller is allowed to pass NULL for both arguments. In 
 *  this case, the function will just return the total number of EMU8010s
 *  without attempting to dereference the array.
 *  
 *
 * @parm DWORD | count | The number of SEID handles in the array.
 *  If 'count' is less than the total number of sound engines, only
 *  the first 'count' IDs will be copied into the array.  If 'count' is 0,
 *  the function will not attempt to fill the array.
 * @parm HRMID * | ids | An array of 'count' HRMID handles.
 *	If NULL, the routine will not attempt to fill the array with IDs.
 * 
 * @rdesc The total number of discovered or partially discovered EMU8010s in the system.  
 * If an error occurs, the function will return 0.
 */
EMUAPIEXPORT DWORD hrmGetHardwareInstances(DWORD dwNum /* VSIZE */, HRMID *ids /* IO */);

/* @func For a given HRM8210 ID, return the FXID. This allows talking to FX8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the FXID
 */
EMUAPIEXPORT DWORD   hrmGetFXID (HRMID id);

/* @func For a given HRM8210 ID, return the SEID. This allows talking to SE8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the SEID
 */
EMUAPIEXPORT DWORD   hrmGetSEID (HRMID id);

/* @func For a given HRM8210 ID, return the HRBID. This allows talking to HRB8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the HRBID
 */
EMUAPIEXPORT DWORD   hrmGetHRBID (HRMID id);

/* @func For a given HRM8210 ID, return the IPID. This allows talking to IP8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the IPID
 */
EMUAPIEXPORT DWORD   hrmGetIPID (HRMID id);

/* @func For a given HRM8210 ID, return the SMID. This allows talking to SM8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the SMID
 */
EMUAPIEXPORT DWORD   hrmGetSMID (HRMID id);

/* @func For a given HRM8210 ID, return the MXRID. This allows talking to MXR8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the MXRID
 */
EMUAPIEXPORT DWORD   hrmGetMXRID (HRMID id);

/* @func For a given HRM8210 ID, return the HALID. This allows talking to HAL8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the HALID
 */
EMUAPIEXPORT DWORD   hrmGetHALID (HRMID id);


/* @func For a given HRM8210 ID, return the ITMID. This allows talking to ITM8210.
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip 
 *
 * @rdesc Returns the ITMID
 */
EMUAPIEXPORT DWORD   hrmGetITMID (HRMID id);

/* @func Wait for a number of wall clock counts to pass, then return.  
 *
 * @parm HRMID | hrmID | 'ID' value returned by hrmDiscoverChip
 * @parm DWORD | count |  Number of wall clocks to wait
 *
 * @rdesc Return of non SUCCESS means the wall clock register could not be read,
 * so proper duration may not have passed.
 */
EMUAPIEXPORT EMUSTAT hrmWaitWallClockCounts(HRMID id, DWORD num);

EMUAPIEXPORT DWORD hrmGetHWRevision(HRMID id);

ENDEMUCTYPE

#endif
