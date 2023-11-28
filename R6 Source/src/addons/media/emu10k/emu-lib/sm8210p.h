/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*
* @doc INTERNAL
* @module sm8210p.h | 
*  This module contains the private definitions used by the Sample Manager.
*  These definitions are only for use by the sample manager and other modules
*  (like the debugger) which require an intimate knowledge of how the sample
*  manager actually works.  Most other modules should only reference things
*  declared in the public header file, sm8210.h.
*
* @iex 
* Revision History:
* Version	Person	Date		Reason
* -------	------	----		---------------------------------- 
*  0.002	JK		Feb 07, 98	Added support for Virtual Page Table Entries
*  0.001	JK		Oct 14, 97	Created.
******************************************************************************
*/

#ifndef __SM8210P_H
#define __SM8210P_H

/*****************************************************************************
 * Includes 
 *****************************************************************************/

#include "hal8210.h"
#include "se8210p.h"

/*****************************************************************************
 * Preprocessor macros 
 *****************************************************************************/

#define REGION_POOL_SIZE        (8192L * MAX_CHIPS)
#define SM_UNDEFINED_PTE        0x0000FBADFL
#define SM_SAMPLE_MEMORY_SIZE   (1L << 25)
#define SM_IN_USE               0xBACEFACEL
#define SM_MIN_FRAG_SIZE		256L
#define SM_NUM_PAGES			0x8

/* Region flags */
#define RF_SMALLOCED	   1    /* Indicates that the sample manager
                                 * originally allocated the backing memory */
#define RF_ALWAYS_LOCKED   2    /* Indicates that the region should be treated
								 * as if the pages in it are always locked */

/* Bit fields in a Virtual Page Table Entry */
#define VPTE_PFN_MASK	        0xFFFFF000L
#define VPTE_LOCK_COUNT_MASK    0xFFL
#define VPTE_REGION_COUNT_SHIFT 0x8
#define VPTE_REGION_COUNT_MASK  0xF00L

/* Macros for converting between physical addresses and PTES */
#define PHYSADDR_TO_PTE(_x) (((DWORD) (_x)) << 1)
#define PTE_TO_PHYSADDR(_x) (((DWORD) (_x)) >> 1)

#define MAKE_SMID(idx)	((0x511DL << 16) | (idx))
#define IS_VALID_SMID(_smid) \
	((((DWORD)(_smid) >> 16) == 0x511D) && \
 	 (smStates[(_smid) & 0xFFL].dwInUse == SM_IN_USE))

#define GET_SMSTINDEX(_smid) ((_smid) & 0xFFL)

#define FIND_SMSTATE(_smid) \
	(IS_VALID_SMID(_smid) ? (&smStates[(_smid) & 0xFF]) : NULL)

#define MAKE_SMHANDLE(_x)  ((SMHANDLE) (_x))
#define FIND_REGION(_x)	   ((stRegion*) (_x))

#define NEXT_FREE(_x) ((_x)->un1.pNextFreeRegion)
#define PREV_REG(_x)  ((_x)->prevRegion)
#define NEXT_REG(_x)  ((_x)->nextRegion)

/* Macros for acquiring a virtual address given a region */
#define GET_VPTE(_preg, _index) (smStates[(_preg)->bySMIndex].pdwVPTETable[_index])
#define GET_VPTE_FROM_SADDR(_preg, _addr) (GET_VPTE(_preg, (_addr) >> 12))
#define SET_VPTE(_preg, _index, _vpte) GET_VPTE(_preg, _index) = _vpte

#define GET_FIRST_VPTE_INDEX(_preg) ((_preg)->dwBaseAddr / SM_PAGE_SIZE)
#define GET_LAST_VPTE_INDEX(_preg) \
	((ALIGN_BASE_ADDR((_preg)->dwBaseAddr) + (_preg)->dwLength - 1) / SM_PAGE_SIZE)

/* Macros for manipulating the various fields of the VPTE */
#define GET_VPTE_VADDR(_vpte) (OSVIRTADDR)(_vpte & VPTE_PFN_MASK)
#define SET_VPTE_VADDR(_vpte, _vaddr) \
	_vpte = (_vpte & ~VPTE_PFN_MASK) | (_vaddr & VPTE_PFN_MASK)

#define GET_VPTE_REGIONCOUNT(_vpte) \
	(BYTE) (((_vpte) >> VPTE_REGION_COUNT_SHIFT) & 0xF)
#define SET_VPTE_REGIONCOUNT(_vpte, _count) \
	(_vpte) = ((_vpte) & ~VPTE_REGION_COUNT_MASK) | (_count << VPTE_REGION_COUNT_SHIFT)

#define GET_VPTE_LOCKCOUNT(_vpte) (BYTE) (_vpte & VPTE_LOCK_COUNT_MASK)
#define SET_VPTE_LOCKCOUNT(_vpte, _count) \
	_vpte = (_vpte & ~VPTE_LOCK_COUNT_MASK) | _count

#define ALIGN_BASE_ADDR(_addr) (_addr&0xffffffe0)

/*****************************************************************************
 * Private structures
 *****************************************************************************/

/* @struct stRegion | The region structure contains the information
 *  needed to track the state of a contiguous region of E8010 sample
 *  memory.  We maintain a doubly-linked list of all of the regions that
 *  have been allocated such that the regions are sorted in increasing order
 *  by the dwBaseAddr field.  This allows us to coalesce consecutive free
 *  blocks whenever a block is freed, as well as traverse a list of all of
 *  the regions in the system.  We don't need to store any kind of virtual
 *  address in the region, since the virtual address can be determined by
 *  indexing into the sm state structure's Virtual PTE table using the 
 *  dwBaseAddr field.
 *
 *  We can potentially create a large number of region structures (tens of
 *  thousands, potentially).  For this reason, we make a real effort to keep
 *  this structure small.  This is the major reason for stuff like storing
 *  the voice list head and next free region in a union.  
 */
typedef struct stRegionTag {
	struct stRegionTag *pNextRegion;
								/* @field A pointer to the next region in the 
								 *  region list.  */
	struct stRegionTag *pPrevRegion;
								/* @field A pointer to the previous region
								 *  in the region list. */
	DWORD    dwBaseAddr;		/* @field Base E8010 sample memory address */
	DWORD    dwLength;			/* @field Length of the region */

	union {
      stVoice *pVoiceListHead;	/* @field A pointer to the first voice on the
								 *  voice reference list */
	  struct stRegionTag *pNextFreeRegion;  /* @field A pointer to the next free region */
	} un1;						/* @field Since the voice and region pointers
								 *  will never be in use simultaneously, we
								 *  stick them into a union  */
	BYTE	 bySMIndex;			/* @field The index of the sample manager
								 *  state entry in smStates.  */
    BYTE	 byFlags;			/* @field Region flags */
    BYTE	 byState;			/* @field The region's current state */
    BYTE     byRefCount;		/* @field The number of voices which reference this reg */
} stRegion;

	
/* @struct stSMState | Encapsulates the state of a particular sample memory
 *  manager instances.  The most important data stored in the state structure
 *  are the physical and virtual page tables.  The physical page table 
 *  (pdwPageTable) is a pointer to the nine pages of contiguous memory which
 *  actually store the page table used by the 8010.  The virtual Page Table
 *  Entry table (pdwVirtPTES) contains information about the virtual addresses
 *  which are used to reference the mapped data.  
 */
typedef struct stsmSMStateTag {
    DWORD     dwInUse;          /* @field If set to SM_IN_USE, this
                                 *  state corresponds to a discovered chip */
    HALID     halid;            /* @field The HAL ID of the actual hardware */
    DWORD    *pdwPageTable;     /* @field a pointer to the base of the page
                                 *  table for this manager.  */
	DWORD    *pdwVPTETable;     /* @field An array of 8192 dwords which contain
								 *  the sample manager defined virtual page
								 *  table entries.  There is one entry in this
								 *  table for each entry in the sample memory
								 *  page table.  A VPTE contains the virtual
								 *  page frame number, the number of referencing
								 *  regions, the number of locked references. */
	OSMEMHANDLE pageTableMemHdl;/* @field The page table's memory handle */
	OSMEMHANDLE virtPTEMemHdl;  /* @field The virtual PTE table memory handle */
    stRegion *pFreeRegionList;  /* @field A linked list of free regions.  The
                                 *  regions on this list aren't in any particular
                                 *  order.  */
    stRegion *pAllRegionList;   /* @field A sorted list of all of the regions,
                                 *  both free and allocated.  The list is sorted
                                 *  by the sample memory base address in 
                                 *  ascending order.  */
    DWORD    dwTotalFreeMem;    /* @field The total amount of free memory  */
    DWORD    dwMaxContig;       /* @field The largest contiguous chunk of free
                                 *  memory.  */
} stSMState;


/*********************************************************************************
 * Routines for snagging internal variables
 *********************************************************************************/

EMUAPIEXPORT stSMState *smpGetSMState();
EMUAPIEXPORT DWORD      smpGetSMCount();
EMUAPIEXPORT EMUSTAT    smpCheckConsistency(SMID id);

#endif /* __SM8010P_H */
