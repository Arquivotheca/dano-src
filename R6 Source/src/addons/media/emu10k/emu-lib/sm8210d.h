/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*
* @doc DISCOVERY
* @module sm8210d.h | 
*   Contains the chip discovery interface for the se8210 module.  This
*   interface is semi-private and is only exposed to the hardware resource
*   manager.
*
* @iex 
* Revision History:
* Version	Person	Date		Reason
* -------	------	----		---------------------------------- 
*  0.001	JK		Oct 14, 97	Created.
******************************************************************************
*/

#ifndef __SM8210D_H
#define __SM8210D_H

#include "datatype.h"
#include "hal8210.h"
#include "cfg8210.h"
#include "sm8210.h"

/* @struct SMCONFIG | The configuration description for the sample
 *  memory manager.  This contents of this are somewhat undefined.
 */
typedef struct stsmConfigTag {
    HRMCONTIGMEMORY  smPageTable;
    DWORD            dwSampleMemorySize;
} SMCONFIG;

/*****************************************************************************
 * Functions
 *****************************************************************************/


/* @func Initialize the sample memory manager's internal global data
 *  structures.  This routine should only be called once, when the 
 *  system first starts up.
 */
EMUAPIEXPORT EMUSTAT smInit();


/* @func Initialize the sample memory manager for a particular E8010 chip.
 *  Calling this routine initializes the page table and sets up
 *  the SM8210's per-chip data structures.  It should only be called once
 *  for any given E8010, unless <f smUndiscoverChip> is later called.
 *
 * @parm DWORD | haldID | Contains the Hardware Abstraction Layer ID of the 
 *  contains the E8010 chip ID  whose sample memory is to be discovered.
 * @parm SMCONFIG * | config | A pointer to a configuration description.
 * @parm SMID * | rid | A pointer to memory for the returned SMID.
 *  Upon successful completion, <f seDiscoverChip> will write the ID of the 
 *  discovered sound engine into the memory at this address.
 *
 * @rdesc Returns SUCCESS if the discovery procedure completes successfully.
 *  Otherwise, one of the following values will be returned:
 *		@flag SMERR_BAD_HANDLE   | No E8010 with the given halID was found.
 *		@flag SMERR_INIT_FAILED | Could not successfully initialize the
 *		 sample memory manager.
 */
EMUAPIEXPORT EMUSTAT smDiscoverChip(HALID halid, SMCONFIG *config, 
                                    SMID *returnedID /* IO */);


/* @func Deallocates any memory associated with the specified SMID and
 *  marks the chip as unusable.
 * 
 * @parm SMID | smid | The ID of the sample memory manager to be shut down.
 * 
 * @rdesc Returns SUCCESS if the manager is shut down without incident.
 */
EMUAPIEXPORT EMUSTAT smUndiscoverChip(SMID smid);


#endif /* __SM8210D_H */
