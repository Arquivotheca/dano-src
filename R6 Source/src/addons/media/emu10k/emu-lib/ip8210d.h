/******************************************************************************
*
*                             Copyright (c) 1997
*                E-mu Systems Proprietary All rights Reserved
*
******************************************************************************/

/******************************************************************************
*
* @doc INTERNAL
* @module ip8210d.h | 
* This file contains the datatypes and functions for chip initialization and
* discovery used by the EMU 8010 interrupt pending manager.
*
* @iex 
* Revision History:
*
* Person              Date          Reason
* ------------------  ------------  --------------------------------------
* Michael Preston     Oct 14, 1997  Separated from ip8210.h.
*
******************************************************************************/

#ifndef __IP8210D_H
#define __IP8210D_H

/****************
* Include files
****************/
#include "ip8210.h"
#include "hal8210.h"

/******************************************************************************
* @contents1 Contents |
*
* @subindex Typedefs & Structures
* @subindex Public Functions
******************************************************************************/

/******************************************************************************
* @contents2 Typedefs & Structures |
* @index type,struct |
******************************************************************************/

/******************************************************************************
* @struct stIPConfig | This structure is passed into the <f ipInit> function.
* It contains the information necessary for the system dependent IP manager to
* register interrupts for this instance of hardware.
******************************************************************************/
typedef struct {
   DWORD opaque32BitHandler; /* @field handler to hardware interrupt */
} stIPConfig;

BEGINEMUCTYPE

/******************************************************************************
* @contents2 Public Functions |
* @index func |
******************************************************************************/

/******************************************************************************
*
* @func  Initializes the interrupt pending manager's internal data structures.
*
* @rdesc Returns SUCCESS if initialization succeeded.
*
******************************************************************************/
EMUSTAT ipInit(void);

/******************************************************************************
*
* @func  Initialize the interrupt pending manager on the specified EMU8010 to a
* known quiescent state.  If the initialization completes successfully, all
* interrupts will be cleared.
*
* @parm  HALID | halID | Contains the Hardware Abstraction Layer ID of the
* E8010 chip whose interrupt pending manager is to be discovered.
* @parm  stIPConfig * | pIPConfig | A pointer to a configuration description.
* @parm  IPID * | ipID | A pointer to memory for the returned interrupt pending
* manager ID.  Upon successful completion, <f ipDiscoverChip> will write the ID
* of the discovered interrupt pending manager into the memory at this address.
*
* @rdesc Returns SUCCESS if the discovery procedure completes successfully.
* Otherwise, returns:
*		@flag IP_INVALID_PARAM    | Invalid parameter.
*		@flag IP_MEM_ALLOC_FAILED | Memory allocation failed.
*
******************************************************************************/
EMUSTAT ipDiscoverChip(HALID halID, stIPConfig *pIPConfig, IPID *ipID);

/******************************************************************************
*
* @func  Removes an instance of the interrupt pending manager for a specific
* instance of hardware.  All interrupts are cleared on the chip, and all
* callbacks are removed.
*
* @parm  IPID | ipID | ID of the interrupt pending manager to remove.
*
* @rdesc  Returns SUCCESS if the removal succeeded.  Otherwise, returns:
*		@flag IP_INVALID_PARAM    | Invalid ipID.
*
******************************************************************************/
EMUSTAT ipUndiscoverChip(IPID ipID);

ENDEMUCTYPE

#endif
