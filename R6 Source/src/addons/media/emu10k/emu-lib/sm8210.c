/*****************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*					
******************************************************************************
* @doc INTERNAL
* @module sm8210.c | 
*  This module contains the implementation of the SM8010 sample memory
*  manager API. The API is fully documented in the EXTERNAL section of 
*  sm8210.h.  Please see that file for more details.
*
*  General Theory of operation:
*  
*  The sample manager uses four primary data structures.  The state structure
*  (stSMState) maintains per-board information for the sample manager.  It
*  contains pointer to the other major data structures and keeps track of
*  global data like the amount of free sample memory and the largest contiguous
*  chunk.
*
*  The sample memory is managed via a list of regions (type stRegion).  A
*  region represents a contiguous chunk of sample memory, and every byte
*  of the 32 MB of sample memory belongs to one and only one region at any
*  point in time.  All regions are kept on a doubly linked list which is sorted
*  in ascending order.
*
*  A region can be in one of four states at any point in time:
*  Invalid, Allocated, Mapped, and Immobile.  Invalid regions are used to
*  store segments of free sample memory.  To improve the performance of sample
*  memory allocation operations, all invalid regions are linked onto a free 
*  list.  Regions in the Allocated state have been allocated to a caller but
*  have not yet had any pages mapped into the sample memory space.  Mapped
*  regions have virtual pages associated with them, but the pages are not
*  yet locked and no physical page table entries have been loaded.  Finally,
*  regions marked as immobile have both locked virtual pages and physical 
*  pages.  Only immobile regions can be used as a source of samples.
*
*  One important thing to keep in mind when examining the sample manager code
*  is that the sample manager and the sound engine work very closely with
*  one another and make use of each other's functions.  This intertwining
*  of functionality becomes particularly nasty when we're trying to free
*  an immobile region.  By definition, a region is immobile if one or more
*  voices are currently using it as a source of sample data.  We obviously
*  can't deallocate a region if it is still in use by voices, so the question
*  becomes what to do in this case.  Probably the best solution would be to
*  return an error.  But that isn't in fact what we do.  What we actually do
*  is call into the sound engine code to stop the voices.  The sound engine
*  then turns around and calls the smMemoryRemoveReference function.  Yuck.
*
*  Another place where the intertwining occurs is in the referencing voice list.
*  Every time a voice references a region, that voice is added to that region's
*  reference list.  The reference list is linked via fields that are actually
*  embedded in the voice, which means that a voice can reference one and only
*  one region at any given point in time.  We could make things more opaque
*  by creating a new linked list member type, but we chose not to for reasons
*  of efficiency.
*
* @iex
*  Revision History:
*  Version	Person      Date		Reason
*  -------   ---------	----------	--------------------------------------
*   0.003		JK		Feb 15, 98  Changed the code in a pretty fundamental
*									way to reduce memory usage.  Added VPTEs.
*   0.002	 	JK		Sep 24, 97	Changed to use the 8210 name scheme.
*   0.001		JK		Sep 01, 97	Initial version
* @end
*****************************************************************************
*/

#ifndef __SM8210_C
#define __SM8210_C

/****************************************************************************
* Includes
******************************************************************************/
#include <stdio.h>   /* For definition of sprintf */
#include <string.h>	 /* For string manipulation functions */

#include "dbg8210.h" /* General debugging and assertion support */
#include "hal8210.h" /* Low-level device register manipulation */
#include "se8210.h"  /* Sound engine public definitions */
#include "se8210p.h" /* Sound engine private defs */
#include "sm8210.h"  /* Sample Manager public defs */
#include "sm8210d.h" /* Sample Manager discovery routine defs */
#include "sm8210p.h" /* Sample Manager private defs */
#include "hrm8210.h" /* Hardware resource manager public defs */
#include "hal8210.h" /* EMU8010 Register specific stuff */ 
#include "os8210.h"	 /* Operating System Abstraction Layer */
#include "emuerrs.h" /* Definition of SUCCESS */


/*****************************************************************************
 *  Global variables
 *****************************************************************************/

/* @globalv  The total number of discovered sample memory managers */
DWORD	smCount = 0;

/* @globalv  An array of sm states, representing all of the discovered
 *  SM8210 modules.
 */
stSMState smStates[MAX_CHIPS];

/* External references to globals we need from the sound engine */
extern stSEState seStates[MAX_CHIPS];
extern DWORD     seCount;

/*****************************************************************************
 * Forward declarations of static functions 
 *****************************************************************************/

static void		 _updateMapRegisters(stRegion *pregion, stVoice *pVoice,
                                     DWORD pte, DWORD index, BOOL bStuff);
static stRegion *_allocRegion();
static stRegion *_splitRegion(stRegion*, DWORD, DWORD);
static void      _freeRegion(stRegion *);
static void      _addToFrontOfFreeList(stRegion *);
static void		 _removeFromFreeList(stRegion *);
static void      _recomputeMaxContig(stSMState *pstate);
static EMUSTAT	 _rdwrMemory(SMHANDLE smh, BYTE *pbyBuffer, DWORD dwOffset,
							 DWORD count, BOOL bDoWrite);

/* Fun routines for manipulating the virtual page table entries */
static EMUSTAT	 _allocateVirtualPages(stRegion *preg);
static void		 _freeVirtualPages(stRegion *preg);
static EMUSTAT	 _lockVirtualPages(stRegion *preg);
static void		 _unlockVirtualPages(stRegion *preg);
static void      _zeroVirtualPages(stRegion *preg);

/*****************************************************************************
 * Functions
 *****************************************************************************/

/*****************************************************************************
 * @func Initialize the system global variables.
 */

EMUAPIEXPORT EMUSTAT
smInit()
{
	WORD	  i;

	smCount = 0;
	memset(smStates, 0, sizeof(stSMState) * MAX_CHIPS);

	/* Make the smState table entries as invalid */
	for (i = 0; i < MAX_CHIPS; i++) 
		smStates[i].dwInUse = 0;

	return SUCCESS;
}


/****************************************************************************
 * @func Initialize a per-chip instance of the manager.  We initialize the
 *  page table and set up a page table reference in the sound engine as well.
 */
EMUAPIEXPORT EMUSTAT
smDiscoverChip(HALID halid, SMCONFIG *config, SMID *retID)
{
	DWORD       dwIndex;            /* Iterator */
	DWORD       dwHCValue;			/* HC save value */
	WORD        wFreeIndex;         /* First free index */
	stSMState * pstate;             /* The current state */
	stRegion *  pFirstReg;          /* The first region for the chip */

	/* Fry this for the timebeing */
	*retID = 0;

	/* See if the chip has already been discovered.  While we're at it,
	 * find an unused entry */
	wFreeIndex = 0xFFFF;
	for (dwIndex = 0; dwIndex < MAX_CHIPS; dwIndex++) {
		if (smStates[dwIndex].dwInUse == SM_IN_USE) {
			if (smStates[dwIndex].halid == halid)
				RETURN_ERROR(SMERR_ALREADY_DISCOVERED);
		} else {
			/* Make a note of the fact this this entry is free */
			if (wFreeIndex == 0xFFFF)
				wFreeIndex = (WORD) dwIndex;
		}
	}

	if (wFreeIndex == 0xFFFF)
		RETURN_ERROR(SMERR_INIT_FAILED);

	/* Allocate a new state */
	pstate = &smStates[wFreeIndex];
	pstate->halid = halid;

	/* First, we set up the page table.  Conveniently, the lower-level driver
	 * actually acquires the memory for us and passes it to us through the
	 * the config structure, so all we need to do is assign it.  */
	pstate->pdwPageTable = (DWORD*) config->smPageTable.osVirtAddr;

	if (pstate->pdwPageTable == NULL) 
		RETURN_ERROR(SMERR_NO_HOST_MEMORY);

	DPRINTF(("smDiscover: page table base address = 0x%lx\n", 
	         pstate->pdwPageTable));
	
	/* Invalidate the page table.  Strangely enough, we actually
	 * allocate nine pages for the page table and initialize all
	 * of the page table entries with the address of the ninth page.
	 * This is done to work around some wierd ass hardware problem on
	 * the E8010 which only shows up under conditions of heavy PCI traffic.
	 */
	ASSERT(config->smPageTable.dwSize >= (sizeof(DWORD) * (SM_NUM_PTES + 1024)));
	for (dwIndex = 0; dwIndex < SM_NUM_PTES + 1024; dwIndex++) 
		pstate->pdwPageTable[dwIndex] = 
				PHYSADDR_TO_PTE(config->smPageTable.osPhysAddr + 
						(sizeof(DWORD) * SM_NUM_PTES));

	/* Program the chip with the page table base address */
	ASSERT(config->smPageTable.osPhysAddr);

	/* Set the hardware control bits to turn off the sound engine cache
	 * and fx engine read. */
	dwHCValue = L8010SERegRead(halid, HC);

	L8010SERegWrite(halid, HC, (dwHCValue|HC_LC));
	LGWrite(halid, PTBA, config->smPageTable.osPhysAddr);
	L8010SERegWrite(halid, HC, (dwHCValue&(~HC_LC)));

	/* Set up the virtual PTE table */
	pstate->pdwVPTETable = (DWORD*) osAllocPages(SM_NUM_PAGES, &pstate->virtPTEMemHdl);
	if (pstate->pdwVPTETable == NULL) 
		RETURN_ERROR(SMERR_NO_HOST_MEMORY)
	else
		memset(pstate->pdwVPTETable, 0x0, sizeof(DWORD) * SM_NUM_PTES);

	if (osLockVirtualRange(pstate->pdwVPTETable, SM_NUM_PAGES, 0, NULL) != SUCCESS)
		RETURN_ERROR(SMERR_NO_HOST_MEMORY);

	/* Set up the initial region */
	if ((pFirstReg = _allocRegion()) == NULL) 
		RETURN_ERROR(SMERR_NO_HOST_MEMORY);

	pstate->dwInUse         = SM_IN_USE;
	pstate->pFreeRegionList = pFirstReg;
	pstate->pAllRegionList  = pFirstReg;

    if (config->dwSampleMemorySize < 0x10000 ||
        config->dwSampleMemorySize > SM_SAMPLE_MEMORY_SIZE)
        config->dwSampleMemorySize = SM_SAMPLE_MEMORY_SIZE;

	pstate->dwTotalFreeMem  = config->dwSampleMemorySize;
	pstate->dwMaxContig     = config->dwSampleMemorySize;

	pFirstReg->pPrevRegion  = NULL;
	pFirstReg->pNextRegion  = NULL;
	pFirstReg->dwBaseAddr   = 0;
	pFirstReg->dwLength     = config->dwSampleMemorySize;
	pFirstReg->byState		= smstateInvalid;
	pFirstReg->bySMIndex    = (BYTE) wFreeIndex;
	pFirstReg->byFlags		= 0;
	NEXT_FREE(pFirstReg)    = NULL;

	/* Finish up */
	*retID = MAKE_SMID(wFreeIndex);
	smCount++;
	return SUCCESS;
}


/*****************************************************************************
 * @func Free up all of the memory consumed by the various data structures
 *  used by this instance of the sample memory manager for the chip.  
 *  This includes deallocating all of the region structures and the page table
 */
EMUAPIEXPORT EMUSTAT
smUndiscoverChip(SMID smid)
{
	stSMState *pstate;		/* A pointer to the current state */
	stRegion  *pCurrReg;

	if ((pstate = FIND_SMSTATE(smid)) == NULL)
		return (SMERR_BAD_HANDLE);

	/* Deallocate the region handles */
	pCurrReg = pstate->pAllRegionList;
	while (pCurrReg != NULL) {
		stRegion *pNextRegion = pCurrReg->pNextRegion;
		_freeRegion(pCurrReg);
		pCurrReg = pNextRegion;
	}

	/* NOTE: The memory for the page table was allocated for us by the
	 * lower-level driver, so we don't want to mess with it.
	 */

	/* Deallocate the virtual PTE table */
	osUnlockVirtualRange(pstate->pdwVPTETable, SM_NUM_PAGES);
	osFreePages(pstate->virtPTEMemHdl);

	pstate->dwInUse = 0;
	pstate->halid   = 0;
	smCount--;

	return SUCCESS;
}


/*****************************************************************************
 * @func Retrieve all of the currently discovered Sample Memory Manager
 *  IDs.
 */
EMUAPIEXPORT DWORD
smGetHardwareInstances(DWORD count, SMID *smids)
{
	WORD w;
	WORD wNumFound = 0;
	 
	for (w = 0; (w < MAX_CHIPS) && (wNumFound < count); w++) {
		if (smStates[w].dwInUse == SM_IN_USE) 
			smids[wNumFound++] = MAKE_SMID(w);
	}

    return smCount;
}


/******************************************************************************
 * @func Synthesize a name string for the specified sound engine.
 */

EMUAPIEXPORT EMUSTAT
smGetModuleName(SMID smid, DWORD count, char *szName)
{
	char  name[64];		/* Name buffer.  */

	DWORD id = GET_SMSTINDEX(smid);

	if (!IS_VALID_SMID(smid)) 
		RETURN_ERROR(SMERR_BAD_HANDLE);

	sprintf(name, "EMU 8210 Sample Memory Manager %d", id);
	strncpy(szName, name, (size_t) count);

	return SUCCESS;
}


/******************************************************************************
 * @func Return attribute information.  Right now, this doesn't do much.
 */
EMUAPIEXPORT EMUSTAT
smGetModuleAttributes(SMID seid, SMATTRIB *attr)
{
	RETURN_ERROR(1);
}


EMUAPIEXPORT EMUSTAT
smGetSampleMemoryStatus(SMID smid, DWORD *retTotalFree, DWORD *retMaxContig)
{
	stSMState *pstate;

	if ((pstate = FIND_SMSTATE(smid)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	*retTotalFree = pstate->dwTotalFreeMem;
	*retMaxContig = pstate->dwMaxContig;

	return SUCCESS;
}


/****************************************************************************
 * @func Compacts memory using the following algorithm: we attempt to slide
 *  all of the allocated samples towards the end of the sample memory in order
 *  to create one large block of unallocated memory in the lower half of 
 *  sample memory (we do it in the lower half in order to make it possible to 
 *  use the memory to satisfy byte-sized sample requests, which must be 
 *  allocated from the first 16 MB of sample memory).  We scan from the end 
 *  of the region list to the beginning.  Every time we encounter an 
 *  unallocated block of memory, we set a pointer to it and then proceed 
 *  to scan forward until we find the next invalid region whose size is >= 8K.
 *  We don't actually slide samples around currently, since this would require
 *  potentially large amounts of copying.  Instead, we just play games with
 *  the page tables and remap the samples.  This has the added advantage of
 *  allowing us to compact mapped as well as host-allocated regions, although
 *  it does mean that we'll get some page internal fragmentation.
 *
 *  Obviously, we can't move pages which are actually being played from.
 *  So when we encounter regions in the Immobile state we just jump the curr
 *  region forward past the immobilized region.  As a result, we can never
 *  compact across an immobilized region.
 *
 *  If we can find two invalid regions which both span at least one full page
 *  and which have no immobilized regions between them, we then go in and 
 *  attempt to move all of the full pages from the higher region to the lower
 *  region.  Depending on the alignment of the higher region, this may or
 *  may not lead to the ultimate removal of the higher invalid region from the
 *  region and free lists.
 */
EMUAPIEXPORT EMUSTAT
smCompactSampleMemory(SMID smid)
{
#if 0
	stSMState *pstate;
	stRegion  *pCurrReg;
	stRegion  *pOtherReg;
    stRegion  *preg;
    DWORD      dwPageOffset;
    DWORD      dwCurrRegLowAddr, dwCurrRegHighAddr;

	if ((pstate = FIND_SMSTATE(smid)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	/* First, we need to find the last region in the list */
	pCurrReg = pstate->pAllRegionList;
    ASSERT(pCurrReg);
	while (pCurrReg->pNextRegion != NULL)
		pCurrReg = pCurrReg->pNextRegion;

	/* Continue scanning until we reach the beginning of the region list */
	while (pCurrReg != NULL) {

		/* Scan backwards until we find an invalid region */
		while (pCurrReg && pCurrReg->byState != smstateInvalid)
			pCurrReg = pCurrReg->pPrevRegion;

		/* Can't find any invalid regions */
		if (pCurrReg == NULL)
			break;

        /* If the size of the invalid region is less than 4K, there's no
         * point in messing with it.  */
        if (pCurrReg->dwLength < 0x1000) {
            pCurrReg = pCurrReg->pPrevRegion;
            continue;
        }

        /* Figure out how many pages we can wring from this region. */
        dwCurrRegLowAddr = (ALIGN_BASE_ADDR(pCurrReg->dwBaseAddr) + (SM_PAGE_SIZE - 1)) & 
            ~(SM_PAGE_SIZE - 1);  /* Round up to higher page boundary */
        dwCurrRegHighAddr = (ALIGN_BASE_ADDR(pCurrReg->dwBaseAddr) + pCurrReg->dwLength) & 
            ~(SM_PAGE_SIZE - 1);  /* Round down to page boundary */
        if (dwCurrRegLowAddr >= dwCurrRegHighAddr) {
            pCurrReg = pCurrReg->pPrevRegion;
            continue;
        }

        dwPageOffset = (dwCurrRegHighAddr - dwCurrRegLowAddr) / SM_PAGE_SIZE;

		/* Okay, we found an invalid region.  Now we scan backwards again 
		 * looking for the next invalid region.  However, if we find an
		 * immobile region between the current region and the previous
		 * invalid region, we can't move stuff around.
		 */
        for (pOtherReg = pCurrReg->pPrevRegion; 
             (pOtherReg != NULL) && (pOtherReg->byState != smstateImmobile) &&
             (pOtherReg->byState != smstateInvalid || pOtherReg->dwLength < 0x2000); 
             pOtherReg = pOtherReg->pPrevRegion)  
                 /* LOOP */ ;

		if (pOtherReg->byState == smstateInvalid && 
            pOtherReg->dwLength >= 0x2000) {
			/* Okay, we found two invalid regions which are separated by
			 * mapped but not immobilized regions.  We should be able to
			 * slide all of the samples up in sample memory and merge the
			 * two regions.
			 */
            DWORD dwLowAddr;
            DWORD dwSrcIndex, dwDestIndex;
            DWORD dwNumPagesToMove;
            DWORD dwSafety;

            dwLowAddr = (ALIGN_BASE_ADDR(pOtherReg->dwBaseAddr) + (SM_PAGE_SIZE - 1)) &
                ~(SM_PAGE_SIZE - 1);

            dwSrcIndex = dwLowAddr / SM_PAGE_SIZE;
            dwDestIndex = dwSrcIndex + dwPageOffset;
            dwNumPagesToMove = (dwCurrRegLowAddr - dwLowAddr) / SM_PAGE_SIZE;

            /* Move the page entries in the VPTE Table and the page table */
        dwSafety = pstate->pdwVPTETable[dwCurrRegHighAddr / SM_PAGE_SIZE];
            memcpy(&(pstate->pdwVPTETable[dwDestIndex]), 
                   &(pstate->pdwVPTETable[dwSrcIndex]), 
                   dwNumPagesToMove * sizeof(DWORD));
        ASSERT(dwSafety == pstate->pdwVPTETable[dwCurrRegHighAddr / SM_PAGE_SIZE]);

            
        dwSafety = pstate->pdwPageTable[dwCurrRegHighAddr / SM_PAGE_SIZE];
            memcpy(&(pstate->pdwPageTable[dwDestIndex]),
                   &(pstate->pdwPageTable[dwSrcIndex]),
                   dwNumPagesToMove * sizeof(DWORD));
        ASSERT(dwSafety == pstate->pdwPageTable[dwCurrRegHighAddr / SM_PAGE_SIZE]);

            /* Now move through the regions and change their base addresses */
            for (preg = pCurrReg; preg != pOtherReg; preg = preg->pPrevRegion)
                preg->dwBaseAddr += (dwPageOffset * SM_PAGE_SIZE);

            /* Make the lower memory region bigger to accomodate the space 
             * that has shifted */
            pOtherReg->dwLength += (dwPageOffset * SM_PAGE_SIZE);

            /* Make the region at the top of memory smaller (to account 
             * for the pages which have been shifted */
            ASSERT(pCurrReg->dwLength >= (dwNumPagesToMove * SM_PAGE_SIZE));
            pCurrReg->dwLength -= (dwPageOffset * SM_PAGE_SIZE);
            if (pCurrReg->dwLength == 0) {
                _removeFromFreeList(pCurrReg);

                if (pCurrReg->pNextRegion)
                    pCurrReg->pNextRegion->pPrevRegion = pCurrReg->pPrevRegion;
                if (pCurrReg->pPrevRegion)
                    pCurrReg->pPrevRegion->pNextRegion = pCurrReg->pNextRegion;

                _freeRegion(pCurrReg);
            }

            /* Clear the newly opened VPTE entries */
            for ( ; dwSrcIndex < dwDestIndex; dwSrcIndex++)
                SET_VPTE(pOtherReg, dwSrcIndex, 0x0);
		}

        ASSERT(smpCheckConsistency(smid) == SUCCESS);

        /* Now proceed onwards with this region */
        pCurrReg = pOtherReg;

    } /* end while */

    _recomputeMaxContig(pstate);
    ASSERT(smpCheckConsistency(smid) == SUCCESS);
#endif

	return SUCCESS;
}


/****************************************************************************
 * @func Allocate a region of sample memory.  This rather frightening function
 *  attempts to allocate memory by searching the free list for regions which
 *  can fulfill the request.  We need to do fairly different things based
 *  on whether the caller wants us to allocate host memory to back the 
 *  region (indicated by SMAF_ALLOC_HOST).  If SMAF_ALLOC_HOST is set,
 *  we can allocate non-page-aligned memory and thus avoid excessive amounts
 *  of internal fragmentation.  If it isn't set, though, we need to insure 
 *  that we allocate sample memory in page-sized chunks.
 */
EMUAPIEXPORT EMUSTAT
smMemoryAllocate(SMID smid, DWORD dwSize, DWORD dwFlags, SMHANDLE *retHandle)
{
	stSMState *pstate;           /* Points to the SMState we're allocing from */
	stRegion  *pCurrRegion;      /* The region currently being examined */
	stRegion  *pPrevRegion;      /* Previous region in the free list */
	stRegion  *pNewRegion;       /* Newly allocated region */
	EMUSTAT    status;

	ASSERT(dwSize != 0);
	ASSERT(retHandle != NULL);
    ASSERT(smpCheckConsistency(smid) == SUCCESS);

    *retHandle = 0;
	pNewRegion = NULL;

	/* Check to see whether the state is valid */
	if ((pstate = FIND_SMSTATE(smid)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	/* Round the size up to the next largest page multiple */
	if (dwFlags & SMAF_ALLOC_HOST)
		dwSize = (dwSize + (dwFlags&SMAF_ALIGN_MASK) + SM_MIN_FRAG_SIZE - 1) &
                     ~(SM_MIN_FRAG_SIZE - 1);
	else
		dwSize = (dwSize + (dwFlags&SMAF_ALIGN_MASK) + SM_PAGE_SIZE - 1) &
                     ~(SM_PAGE_SIZE - 1);

	/* See if there is enough total memory to satisfy the request */
	if (dwSize > pstate->dwTotalFreeMem) 
		RETURN_ERROR(SMERR_NO_SAMPLE_MEMORY);

	/* Tell the user about fragmentation problems, if any */
	if (dwSize > pstate->dwMaxContig) 
		RETURN_ERROR(SMERR_SAMPLE_MEMORY_FRAGMENTED);

	/* Go look for a region.  We don't know for sure that we'll be able to
	 * find one, since it is possible that whatever is out there won't be
	 * aligned well, but we'll look.
	 */
	for (pPrevRegion = NULL, pCurrRegion = pstate->pFreeRegionList; 
		 pCurrRegion != NULL; 
		 pPrevRegion = pCurrRegion, 
	     pCurrRegion = pCurrRegion->un1.pNextFreeRegion)
	{
		/* Move on if the region's too small */
		if (pCurrRegion->dwLength < dwSize)
			continue;

		/* If we're asking for an 8-bit sample and this region doesn't
		 * start below 16 Megs, skip it. */
		if ((dwFlags & SMAF_8BIT_DATA) && 
			(pCurrRegion->dwBaseAddr > (SM_SAMPLE_MEMORY_SIZE / 2)))
			continue;

		/* Check to see whether we're asking for a region which will be
		 * backed by data allocated by the sample manager.
		 */
		if (dwFlags & SMAF_ALLOC_HOST) 
		{
			/* Try and chop the region into two pieces and return
			 * a new region of the requested size */
			pNewRegion = _splitRegion(pCurrRegion, dwSize, dwFlags);
			if (pNewRegion == NULL)
				continue;

			*retHandle = MAKE_SMHANDLE(pNewRegion);

			/* Allocate the memory for the region */
			pNewRegion->byFlags = RF_SMALLOCED;

			if ((status = _allocateVirtualPages(pNewRegion)) != SUCCESS) {
				smMemoryFree(MAKE_SMHANDLE(pNewRegion));
				RETURN_ERROR(SMERR_NO_HOST_MEMORY);
			}

			/* Lock the region into memory if desired */
			/* FIXME XXX HACK When we get around to handling locking and 
			 * unlocking in the upper level code we need to remove this.  
			 * In the mean time, however, we need it, since not doing this
			 * causes a nasty hesitation if the sample gets paged out.  */
			//if (dwFlags & SMAF_LOCK) {
				pNewRegion->byFlags |= RF_ALWAYS_LOCKED;
                if (_lockVirtualPages(pNewRegion) != SUCCESS) {
                    smMemoryFree(*retHandle);
                    *retHandle = 0;
                    return SMERR_NO_HOST_MEMORY;
                }
			// }

            DPRINTF(("smMemoryAllocate: New Region 0x%lx, base addr %ld  len %ld", 
                     pNewRegion, pNewRegion->dwBaseAddr, pNewRegion->dwLength));

            pNewRegion->dwBaseAddr += dwFlags&SMAF_ALIGN_MASK;

			ASSERT(smpCheckConsistency(smid) == SUCCESS);
			return SUCCESS;
		} 
		else  /* Allocate mapped case */
		{
			/* In this case, we're trying to allocate a region which
			 * will have pages mapped into it by the caller.  This
			 * case is slightly more complex, because we must insure
			 * that the region we hand back begins on a page-aligned
			 * boundary.
			 */
			DWORD dwAlignedBaseAddr;
            DWORD dwFragmentSize;
			DWORD dwAlignedLength;

			dwAlignedBaseAddr = (pCurrRegion->dwBaseAddr + SM_PAGE_SIZE - 1) & 
				~(SM_PAGE_SIZE - 1);

            dwFragmentSize = dwAlignedBaseAddr - pCurrRegion->dwBaseAddr;
			dwAlignedLength = pCurrRegion->dwLength - dwFragmentSize;

			if (dwAlignedLength >= dwSize) {

				/* If there are any fragments preceeding the current block,
				 * transfer them to the previous region */
				if (pCurrRegion->dwBaseAddr < dwAlignedBaseAddr) {
					stRegion* prevreg = pCurrRegion->pPrevRegion;

					/* This situation shouldn't be possible unless we have
					 * a predecessor, but... */
					ASSERT(prevreg);
					prevreg->dwLength += dwFragmentSize;

                    /* If we're giving the memory to an allocated region,
                     * we have to update the free memory count */
                    if (prevreg->byState != smstateInvalid)
                        pstate->dwTotalFreeMem -= dwFragmentSize;

					pCurrRegion->dwBaseAddr = dwAlignedBaseAddr;
					pCurrRegion->dwLength = dwAlignedLength;

                    ASSERT(((pCurrRegion->pNextRegion) ? 
                           ((pCurrRegion->dwBaseAddr + pCurrRegion->dwLength) == pCurrRegion->pNextRegion->dwBaseAddr)
                           : 1));
                }

				/* At this point, the region is all nice and aligned, so
				 * we can use splitRegion to do the rest of the work.  We
				 * explicitly use the SMAF_8BIT_DATA flag to ensure that
				 * we get the block from the beginning, since we need this
				 * to ensure */
				pNewRegion = _splitRegion(pCurrRegion, dwSize, SMAF_8BIT_DATA);
                if (pNewRegion == NULL) {
                    *retHandle = 0;
                    RETURN_ERROR(SMERR_NO_HOST_MEMORY);
                } else {
                    *retHandle = MAKE_SMHANDLE(pNewRegion);
                }
			}
		}

        pNewRegion->dwBaseAddr += dwFlags&SMAF_ALIGN_MASK;

		ASSERT(smpCheckConsistency(smid) == SUCCESS);
		return SUCCESS;
	} /* end for loop */

	/* Couldn't find any memory.  Since we checked up above to see if
	 * their was enough gross memory lying around, we must not be able
	 * to find enough of the right kind of memory.  Maybe we're asking for
	 * an 8-bit region and there isn't enough space in the low 16 megs.  Maybe
	 * we're asking for a mappable region and their aren't aligned pages.
	 */
	RETURN_ERROR(SMERR_SAMPLE_MEMORY_FRAGMENTED);
}


/* @func Try and chop the given region into a new region of the specified
 *  size.  If the desired size is the same as the size of the region,
 *  then we just return the specified region.  This is mostly just used as
 *  a helper function for smMemoryAllocate.
 */
stRegion *
_splitRegion(stRegion *preg, DWORD dwSize, DWORD dwFlags)
{
	stSMState *pstate = &smStates[preg->bySMIndex];
	stRegion  *pNewRegion;
	DWORD      dwOrigRegionSize = preg->dwLength;

	if (preg->dwLength == dwSize) 
	{
		/* Just use this block */
		pNewRegion = preg;
		_removeFromFreeList(pNewRegion);
	} 
	else
	{
		/* We need to chop the region into two pieces; grab a new region. */
		pNewRegion = _allocRegion();
		if (pNewRegion == NULL)
			RETURN_ERROR(NULL);

		/* In order to maximize the amount of space available in the first
		 * 16 megs, we always try and allocate 16-bit sample blocks from the
		 * end of the region.  Early on, when there are just a few large regions
		 * on the free list, this results in keeping the low 16 megs free.  */
		if (dwFlags & SMAF_8BIT_DATA) 
		{
			pNewRegion->dwBaseAddr = preg->dwBaseAddr;
			pNewRegion->dwLength   = dwSize;
			
			preg->dwBaseAddr = preg->dwBaseAddr + dwSize;
			
			/* Link the new region into the region list */
			pNewRegion->pNextRegion = preg;
			pNewRegion->pPrevRegion = preg->pPrevRegion;
			if (preg->pPrevRegion)
				preg->pPrevRegion->pNextRegion = pNewRegion;
			else
				pstate->pAllRegionList = pNewRegion;
			
			preg->pPrevRegion = pNewRegion;
		}
		else
		{
			pNewRegion->dwBaseAddr = preg->dwBaseAddr + (preg->dwLength - dwSize);
			pNewRegion->dwLength   = dwSize;
			
			/* Link the new region into the region list after preg */
			pNewRegion->pNextRegion = preg->pNextRegion;
			pNewRegion->pPrevRegion = preg;
			if (preg->pNextRegion)
				preg->pNextRegion->pPrevRegion = pNewRegion;
			preg->pNextRegion = pNewRegion;
		}
		
		preg->dwLength -= dwSize;
	}

	/* Just initialize the region */
	pNewRegion->un1.pVoiceListHead = NULL;
	pNewRegion->bySMIndex  = preg->bySMIndex;
	pNewRegion->byFlags    = 0;
	pNewRegion->byState    = smstateAllocated;
	pNewRegion->byRefCount = 0;

	/* Update the state's memory statistics */
	pstate->dwTotalFreeMem -= dwSize;

	if (pstate->dwMaxContig < (dwOrigRegionSize + 2*SM_PAGE_SIZE)) {
		/* We just split up a block which could have been the largest 
         * contiguous block (or pretty close). Now we need to recompute
		 * the Max Contig value.  The fuzziness here comes from the fact
         * that previous operations may have done some alignment which altered
         * the size of the block.
		 */
        _recomputeMaxContig(pstate);
    }

    ASSERT(smpCheckConsistency(MAKE_SMID(pNewRegion->bySMIndex)) == SUCCESS);

	return pNewRegion;
}


/****************************************************************************
 * @func Free the region of sample memory referenced by the given handle.
 *  During the free operation we try and coalesce the newly freed region
 *  with any neighboring regions which are also free.
 */

EMUAPIEXPORT EMUSTAT
smMemoryFree(SMHANDLE smh)
{
	stSMState *pstate;
	stRegion  *pregion;      /* A pointer to the region to free */
	stRegion  *pPrevRegion;  /* The preceeding region in the region list */
	stRegion  *pNextRegion;  /* The next region in the region list */
    stVoice   *pvoice;       /* A pointer to the voice we gonna shut down */
    BOOL       bOnFreeList;  /* Indicates whether pregion is on the free list */

	/* Acquire the region pointer from the handle */
	if ((pregion = FIND_REGION(smh)) == NULL) 
		RETURN_ERROR(SMERR_BAD_HANDLE);

    ASSERT(smpCheckConsistency(MAKE_SMID(pregion->bySMIndex)) == SUCCESS);

    DPRINTF(("smMemoryFree region 0x%lx  base %ld len %ld", 
            pregion, pregion->dwBaseAddr, pregion->dwLength));

	/* Check to see if the pages are always locked */
	if (pregion->byFlags & RF_ALWAYS_LOCKED)
		_unlockVirtualPages(pregion);

	pstate = &smStates[pregion->bySMIndex];

	/* If the region is IMMOBILE, someone must still be using it.
	 * We'll try stopping all of the voices on the reference list first.  */
	if (pregion->byState == smstateImmobile) {
		pvoice = pregion->un1.pVoiceListHead;
		while (pvoice != NULL) {
			SEVOICE voice = MAKE_SEVOICE_FROM_PTR(pvoice);
			stVoice *nextVoice;

			/* seVoiceStop will in turn call smMemoryRemoveReference to remove
			 * the voice from the pregion's linked list of voices, so grab 
			 * hold of the nextVoice pointer first.  */
			nextVoice = pvoice->nextVoice;

			seVoiceStop(1, &voice);

			/* Make sure that we've completely cleaned up the voice */
			pvoice->nextVoice = NULL;
			pvoice->prevVoice = NULL;
			pvoice->byNumRefs = 0;

			pvoice = nextVoice;
		}
	}

	/* If the region is mapped and the sample memory manager originally
	 * allocated the memory then we need to unmap it now.  */
	if (pregion->byFlags & RF_SMALLOCED) {
		_freeVirtualPages(pregion);
		pregion->byFlags = 0;
	} else {
		/* Just zero the page table entries, since the caller was responsible
		 * for allocating them initially.  */
		_zeroVirtualPages(pregion);
	}

	ASSERT(pregion->byRefCount == 0);

	/* Update the total amount of free memory now, since the original
	 * size of the region is harder to figure out after coalescing */
	ASSERT(pstate->dwInUse == SM_IN_USE);
	pstate->dwTotalFreeMem += pregion->dwLength;

	/* See if our predecessor on the region list is free.  If so, coalesce */
	pPrevRegion = pregion->pPrevRegion;
	ASSERT(!pPrevRegion || (pPrevRegion->pNextRegion == pregion));

	/* At first, we know the the block isn't on the free list */
	bOnFreeList = FALSE;

    /* Realign base address */
    pregion->dwBaseAddr = ALIGN_BASE_ADDR(pregion->dwBaseAddr);

    if (pPrevRegion && pPrevRegion->byState == smstateInvalid) {
		/* It seems to be free; unify these blocks into a single region 
		 * pointed to be pregion */

		pPrevRegion->dwLength += pregion->dwLength;

		/* Remove pregion from the region list */
		pPrevRegion->pNextRegion = pregion->pNextRegion;
		if (pregion->pNextRegion) 
			pregion->pNextRegion->pPrevRegion = pPrevRegion;
        
		/* Get rid of the region and update the pregion pointer */
		_freeRegion(pregion);
		pregion = pPrevRegion;

		/* Since we just merged the freed region into its predecessor,
		 * and the predecessor must have been on the free list, we 
		 * know that pregion block is now on the free list.  */
		bOnFreeList = TRUE;
	}

    /* See if the successor is free.  If so, merge it in as well */
	pNextRegion = pregion->pNextRegion;
	ASSERT(!pNextRegion || pNextRegion->pPrevRegion == pregion);

	if (pNextRegion && pNextRegion->byState == smstateInvalid) {
		
		if (bOnFreeList) {
			/* We're coalescing two free blocks which are both on the
			 * free list into a single block, so we need to remove the
			 * first block from the free list */
			_removeFromFreeList(pregion);
        }

		/* Remove the old region from the region linked list. First set
		 * our successor's predecessor to our predecessor. */
		pNextRegion->pPrevRegion = pregion->pPrevRegion;

		/* Next, set our predecessor's successor pointer to our successor */
		if (pregion->pPrevRegion == NULL) 
			pstate->pAllRegionList = pNextRegion;
		else
			pregion->pPrevRegion->pNextRegion = pNextRegion;

		/* Merge the regions */
		pNextRegion->dwBaseAddr = pregion->dwBaseAddr;
		pNextRegion->dwLength  += pregion->dwLength;
		bOnFreeList = TRUE;

		/* Finally, get rid of the pregion */
		_freeRegion(pregion);
		pregion = pNextRegion;
	}

	/* If the region hasn't yet been added to the free list, add it now. */
	if (!bOnFreeList) {
		pregion->byState = smstateInvalid;
		_addToFrontOfFreeList(pregion);
	}

	/* Update the state's value for the largest free region */
	if (pregion->dwLength > pstate->dwMaxContig)
		pstate->dwMaxContig = pregion->dwLength;

	ASSERT(smpCheckConsistency(MAKE_SMID(pregion->bySMIndex)) == 0);
	return SUCCESS;
}


/****************************************************************************
 * @func Add a reference to the SM handle.  
 */
EMUAPIEXPORT EMUSTAT
smMemoryAddReference(SMHANDLE smh, SEVOICE sevoice)
{
	stRegion *pregion;
    stVoice  *pvoice;
	BOOL      bIncrementCount = FALSE;

	if ((pregion = FIND_REGION(smh)) == NULL) 
		RETURN_ERROR(SMERR_BAD_HANDLE);

	if (sevoice != 0) {
		CHECK_VOICE(pvoice, sevoice);

		ASSERT(pvoice->smh == smh);

		/* Check to see if the voice is already referenced */
	    if ((pvoice != pregion->un1.pVoiceListHead) &&
			(pvoice->prevVoice == NULL)) {
			/* Voice isn't already on list, so add it */
			pvoice->prevVoice = NULL;
		    pvoice->nextVoice = pregion->un1.pVoiceListHead;
			pregion->un1.pVoiceListHead = pvoice;

			if (pvoice->nextVoice != NULL) 
				pvoice->nextVoice->prevVoice = pvoice;

			bIncrementCount = TRUE;
		}

		ASSERT(pvoice->byNumRefs < 255);
		pvoice->byNumRefs++;
	} else {
		bIncrementCount = TRUE;
	}
	
	if (bIncrementCount) {

		/* Increment the count of referencing voices */
		ASSERT(pregion->byRefCount < 255);
		pregion->byRefCount++;

		/* Mark all of the referenced pages in the virtual page table as 
		 * being in use */
        if (_lockVirtualPages(pregion) != SUCCESS) {
            RETURN_ERROR(SMERR_NO_HOST_MEMORY);
        }
	}

	pregion->byState = smstateImmobile;

	/* We insure that the map registers for the voice are programmed
	 * correctly.  */
	_updateMapRegisters(pregion, pvoice, pregion->dwBaseAddr >> 12, 
						pregion->dwLength >> 12, TRUE);

	return SUCCESS;
}


/****************************************************************************
 * @func Remove a reference from the specified region.
 */
EMUAPIEXPORT EMUSTAT
smMemoryRemoveReference(SMHANDLE smh, SEVOICE sevoice)
{
	stRegion *pregion;
	stVoice  *pvoice;

	if ((pregion = FIND_REGION(smh)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	ASSERT(pregion->byRefCount > 0);

	if (sevoice != 0) {
		CHECK_VOICE(pvoice, sevoice);

		ASSERT(pvoice->smh == smh);
	    ASSERT(pvoice->byNumRefs > 0);

		pvoice->byNumRefs--;
	
		if (pvoice->byNumRefs == 0) {
			/* Remove the voice from the reference list */
			if (pvoice->prevVoice == NULL) 
				pregion->un1.pVoiceListHead = pvoice->nextVoice;
			else
				pvoice->prevVoice->nextVoice = pvoice->nextVoice;

			if (pvoice->nextVoice != NULL)
				pvoice->nextVoice->prevVoice = pvoice->prevVoice;

			pvoice->prevVoice = NULL;
			pvoice->nextVoice = NULL;
		}
	}

	if ((pvoice->byNumRefs == 0) || (sevoice == 0)) {

		/* Decrement the count of referencing voices */
		pregion->byRefCount--;

		/* Unlock the virtual pages; note this may in fact not actually
		 * unlock the linear address if other regions are referring to the
 		 * same page. */
		_unlockVirtualPages(pregion);
	}

	if (pregion->byRefCount == 0)
		pregion->byState = smstateMapped;

	return SUCCESS;
}


/****************************************************************************
 * @func Returns the current state of the specified region.
 */
EMUAPIEXPORT EMUSTAT
smMemoryGetStatus(SMHANDLE smh, SMSTATEVALUE *retStatus)
{
	stRegion *pregion;

	if ((pregion = FIND_REGION(smh)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	*retStatus = (SMSTATEVALUE) pregion->byState;
	return SUCCESS;
}


/****************************************************************************
 * @func Stuff an SMHANDLEINFO structure full of information about the
 *  specified region.
 */
EMUAPIEXPORT EMUSTAT
smMemoryGetInfo(SMHANDLE smh, SMHANDLEINFO *retInfo)
{
	stRegion *pregion;
	stVoice  *pvoice;
	WORD	  wIndex;

	if ((pregion = FIND_REGION(smh)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	ASSERT(retInfo != NULL);

	retInfo->state      = (SMSTATEVALUE) pregion->byState;
	retInfo->dwBaseAddr = pregion->dwBaseAddr;
	retInfo->dwSize     = pregion->dwLength -
        (pregion->dwBaseAddr - ALIGN_BASE_ADDR(pregion->dwBaseAddr));

	/* Stuff the voices array with the referencing voices */
	pvoice = pregion->un1.pVoiceListHead;
	wIndex = 0;
	while (pvoice != NULL && wIndex < 64) {
		retInfo->voices[wIndex++] = MAKE_SEVOICE_FROM_PTR(pvoice);
		pvoice = pvoice->nextVoice;
	}
	retInfo->dwNumVoices = wIndex;

	return SUCCESS;
}


/***************************************************************************
 * @func Retrieves the base and length for the specified region.
 */
EMUAPIEXPORT EMUSTAT
smMemoryGetBaseAndLength(SMHANDLE smh, DWORD *retBaseAddr, DWORD *retLength)
{
	stRegion  * pregion;

	if ((pregion = FIND_REGION(smh)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	if (retBaseAddr)
		*retBaseAddr = pregion->dwBaseAddr;

	if (retLength)
		*retLength   = pregion->dwLength -
            (pregion->dwBaseAddr - ALIGN_BASE_ADDR(pregion->dwBaseAddr));

	return SUCCESS;
}


/***************************************************************************
 * @func Fills in the given SMMAP structure.
 */
EMUAPIEXPORT EMUSTAT
smMemoryGetMap(SMHANDLE smh, SMMAP *map)
{
	stRegion  *pregion;
	stSMState *pstate;
	DWORD      dwPageCount;
	DWORD	   dwPageIndex, dwPageStartIndex;

	if ((pregion = FIND_REGION(smh)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	if (pregion->byState != smstateMapped)
		RETURN_ERROR(SMERR_NO_MAPPING);

	pstate = &smStates[pregion->bySMIndex];
	ASSERT(pstate->dwInUse == SM_IN_USE);

	dwPageCount = ((map->dwLength > pregion->dwLength) ? pregion->dwLength
				  : map->dwLength) / SM_PAGE_SIZE;
	dwPageStartIndex = pregion->dwBaseAddr / SM_PAGE_SIZE;
	map->pBaseAddr = (BYTE*) GET_VPTE_VADDR(GET_VPTE_FROM_SADDR(pregion, 
		pregion->dwBaseAddr));
	map->dwLength  = pregion->dwLength;
	
	for (dwPageIndex = 0; dwPageIndex < dwPageCount; dwPageIndex++) {
		DWORD pte = pstate->pdwPageTable[dwPageIndex + dwPageStartIndex];
		map->pdwPhysicalPageList[dwPageIndex] = PTE_TO_PHYSADDR(pte);
	}

	return SUCCESS;
}


EMUAPIEXPORT EMUSTAT
smUpdateMapRegisters(SMHANDLE smh, BYTE byVoiceIndex, DWORD dwCurPageIndex,
                     DWORD dwNextPageIndex)
{
	stSMState *pstate;
	stRegion  *pregion;
    DWORD dwMapValue, dwMapIndex;

	if ((pregion = FIND_REGION(smh)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	pstate = &smStates[pregion->bySMIndex];

    dwMapValue = (pstate->pdwPageTable[dwNextPageIndex]&0xffffe000) |
                 dwNextPageIndex;

    dwMapIndex = LSEPtrRead(pstate->halid, byVoiceIndex|MAPA)&0x1fff;
    /* If they're equal, set MAPB, otherwise set MAPA */
    if (dwCurPageIndex == dwMapIndex)
        LSEPtrWrite(pstate->halid, byVoiceIndex|MAPB, dwMapValue);
    else
        LSEPtrWrite(pstate->halid, byVoiceIndex|MAPA, dwMapValue);

    return SUCCESS;
}

/****************************************************************************
 * @func Read sample data from the specified region.
 */
EMUAPIEXPORT EMUSTAT
smMemoryRead(SMHANDLE smh, BYTE *pbyBuffer, DWORD dwOffset, DWORD dwCount)
{
	return _rdwrMemory(smh, pbyBuffer, dwOffset, dwCount, FALSE);
}


EMUAPIEXPORT EMUSTAT
smMemoryWrite(SMHANDLE smh, BYTE *pbyBuffer, DWORD dwOffset, DWORD dwCount)
{
	return _rdwrMemory(smh, pbyBuffer, dwOffset, dwCount, TRUE);
}


static EMUSTAT
_rdwrMemory(SMHANDLE smh, BYTE *pbyBuffer, DWORD dwOffset, DWORD dwCount, 
			BOOL bDoWrite)
{
	stSMState *pstate;
	stRegion  *pregion;

	if ((pregion = FIND_REGION(smh)) == NULL) 
		RETURN_ERROR(SMERR_BAD_HANDLE);
	
	/* Validate the params */
	if (pregion->dwLength -
        (pregion->dwBaseAddr - ALIGN_BASE_ADDR(pregion->dwBaseAddr)) <
        (dwOffset + dwCount))
		RETURN_ERROR(SMERR_OUT_OF_BOUNDS);

	if (pbyBuffer == NULL)
		RETURN_ERROR(SMERR_BAD_ADDRESS);

	if (pregion->byState != smstateMapped && 
		pregion->byState != smstateImmobile) 
		RETURN_ERROR(SMERR_NO_MAPPING);

	pstate = &smStates[pregion->bySMIndex];
	ASSERT(pstate->dwInUse == SM_IN_USE);

	dwOffset += pregion->dwBaseAddr;

	while (dwCount > 0) {
		DWORD  dwVirtAddr;
		DWORD  dwNumToCopy;

		/* Look up the virtual address of the start of the page */
		dwVirtAddr =  (DWORD) GET_VPTE_VADDR(pstate->pdwVPTETable[dwOffset / SM_PAGE_SIZE]);
		ASSERT(dwVirtAddr && !(dwVirtAddr & 0xFFF));

		/* Increment into the page by the appropriate amount */
		dwVirtAddr += (dwOffset & (SM_PAGE_SIZE - 1));

		dwNumToCopy = SM_PAGE_SIZE - (dwOffset & (SM_PAGE_SIZE - 1));
		dwNumToCopy = ((dwCount < dwNumToCopy) ? dwCount : dwNumToCopy);

		if (bDoWrite)
			memcpy((void*) dwVirtAddr, pbyBuffer, (size_t) dwNumToCopy);
		else
			memcpy(pbyBuffer, (void*) dwVirtAddr, (size_t) dwNumToCopy);

		dwOffset  += dwNumToCopy;
		pbyBuffer += dwNumToCopy;
		dwCount   -= dwNumToCopy;
	}

	return SUCCESS;
}


/***************************************************************************
 * @func Map the given memory range.
 */
#define CHUNK_PAGES 8

EMUAPIEXPORT EMUSTAT
smMemoryMap(SMHANDLE smh, SMMAP *map)
{
	stRegion   *region;
	stSMState  *pstate;
	stVoice    *pCurrVoice;
	DWORD       dwVirtAddr;          
	DWORD       dwStartIndex;
	DWORD		dwOrigStartIndex;
	DWORD       dwIndex;    
	DWORD       dwNumPages;
	DWORD		dwOrigNumPages;
	DWORD       dwVPTE = 0;
    OSPHYSADDR	physAddrs[CHUNK_PAGES];


	if ((region = FIND_REGION(smh)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);

	if (map == NULL)
		RETURN_ERROR(SMERR_BAD_ADDRESS);

	pstate = &smStates[region->bySMIndex];
	ASSERT(pstate->dwInUse == SM_IN_USE);

	dwStartIndex = (region->dwBaseAddr + map->dwOffset) / SM_PAGE_SIZE;
	dwOrigStartIndex = dwStartIndex;
	dwVirtAddr = (DWORD) map->pBaseAddr;
   dwNumPages = (dwVirtAddr + map->dwLength + SM_PAGE_SIZE - 1)/SM_PAGE_SIZE - 
				dwVirtAddr/SM_PAGE_SIZE;
	dwOrigNumPages = dwNumPages;


	if (map->dwFlags & SMMAP_VIRTUAL) {

		ASSERT((dwVirtAddr & (SM_PAGE_SIZE - 1)) == 0);

		DPRINTF(("\nsmMemoryMap: Mapping virtual addr 0x%lx, %ld bytes\n", 
		         dwVirtAddr, map->dwLength));

		/* Check for bounds violation */
		if ((map->dwOffset + map->dwLength) > region->dwLength)
			RETURN_ERROR(SMERR_OUT_OF_BOUNDS);

		/* We doing the virtual-to-physical translation in chunks, so we just
		 * keep iterating until we're done.	 */
		while (dwNumPages > 0) {
			DWORD dwNumToMap = (CHUNK_PAGES < dwNumPages) ? CHUNK_PAGES : dwNumPages;

			if (osLockVirtualRange((OSVIRTADDR) dwVirtAddr,
                                dwNumToMap, dwNumToMap, physAddrs) != SUCCESS)
				RETURN_ERROR(SMERR_MAPPING_FAILED);

			for (dwIndex = 0; dwIndex < dwNumToMap; dwIndex++) {

                /* We were having a problem with the physAddrs array.
                 * It's allocated as a near pointer, but some functions
                 * think it's a far pointer.  For some reason, casting
                 * it to a (DWORD *) here corrected the problem, though
                 * I'm not sure if this is really the right thing to
                 * do.  MRP
				 */
				pstate->pdwPageTable[dwStartIndex + dwIndex] =
					PHYSADDR_TO_PTE(((DWORD *)physAddrs)[dwIndex] & 
                                                  ~(SM_PAGE_SIZE - 1));

				/* Write the appropriate value into the VPTE.  You
				 * might ask yourself, "why set the lock count to 2?" 
				 * Basically, we already know that the pages are locked.
				 * We don't want to lock them again, since this will lead
				 * to a slow but sure locking of many physical pages. By
				 * setting the lock count to two we know that any further
				 * calls to lock the pages will just increment the lock
				 * count again.  Okay, so why two rather than one?  Well,
				 * conversely, when we end up calling smMemoryRemoveReference,
				 * we don't want to unlock the pages.  */
				SET_VPTE_VADDR(dwVPTE, dwVirtAddr + dwIndex * SM_PAGE_SIZE);
				SET_VPTE_REGIONCOUNT(dwVPTE, 1);
				SET_VPTE_LOCKCOUNT(dwVPTE, 2);
				SET_VPTE(region, dwStartIndex + dwIndex, dwVPTE);
			}

     
     		osUnlockVirtualRange((OSVIRTADDR) dwVirtAddr, dwNumToMap);

			dwVirtAddr += (dwNumToMap * SM_PAGE_SIZE);
			dwNumPages -= dwNumToMap;
			dwStartIndex += (WORD) dwNumToMap;
		}

	} else {
		/* Check the bounds */
		if ((map->dwOffset + map->dwLength) >region->dwLength)
			RETURN_ERROR(SMERR_OUT_OF_BOUNDS);

		/* We already have the physical pages, just go for it */
		//dwOrigNumPages = dwNumPages = map->dwLength;
		for (dwIndex = 0; dwIndex < dwNumPages; dwIndex++) {
			/* Write the appropriate value into the VPTE */
			SET_VPTE_VADDR(dwVPTE, dwVirtAddr + dwIndex * SM_PAGE_SIZE);
			SET_VPTE_REGIONCOUNT(dwVPTE, 1);
			SET_VPTE_LOCKCOUNT(dwVPTE, 2);
			SET_VPTE(region, dwStartIndex + dwIndex, dwVPTE);

			pstate->pdwPageTable[dwIndex + dwStartIndex] = 
				PHYSADDR_TO_PTE(map->pdwPhysicalPageList[dwIndex]);
		}
	}

//#if 0
	/* Update the map registers of any playing voice */
    pCurrVoice = region->un1.pVoiceListHead;
	while (pCurrVoice != NULL) {
		_updateMapRegisters(region, pCurrVoice, dwOrigStartIndex, 
			dwOrigNumPages, FALSE);
		pCurrVoice = pCurrVoice->nextVoice;
	}
//#endif

	if (region->byState == smstateAllocated)
		region->byState = smstateMapped;

	return SUCCESS;
}


/****************************************************************************
 * Static functions
 ****************************************************************************/

/****************************************************************************
 * @func Allocate a new region structure.  In an ideal world, we'd use a zone 
 * allocator here.
 */
static stRegion *
_allocRegion(void)
{
	stRegion * region;

	if ((region = (stRegion*) osLockedHeapAlloc(sizeof(stRegion))) == NULL)
		return NULL;

    region->pPrevRegion = NULL;
	region->pNextRegion = NULL;
	region->dwBaseAddr = 0;
	region->dwLength   = 0;
	region->un1.pNextFreeRegion = NULL;
	region->byFlags    = 0;
	region->byState    = smstateInvalid;
	region->byRefCount = 0;

	return region;
}


/****************************************************************************
 * @func Free a region structure.  See above comment about zones.
 */
static void
_freeRegion(stRegion *region)
{
	/* Invalidate the region data */
	region->pNextRegion = NULL;
	region->pPrevRegion = NULL;
	region->dwLength    = 0;
	region->dwBaseAddr  = 0;
	region->byState     = 0xFF;

	osLockedHeapFree(region, sizeof(stRegion));
}


/****************************************************************************
 * @func Recompute the size of the maximum contiguous region of free memory.
 */
static void
_recomputeMaxContig(stSMState *pstate)
{
    stRegion *pCurrRegion;
    DWORD     dwCurrMaxContig;
	
	pCurrRegion = pstate->pFreeRegionList;
	dwCurrMaxContig = 0;
	while (pCurrRegion != NULL) {
		if (pCurrRegion->dwLength > dwCurrMaxContig)
			dwCurrMaxContig = pCurrRegion->dwLength;
		pCurrRegion = pCurrRegion->un1.pNextFreeRegion;
	}
	pstate->dwMaxContig = dwCurrMaxContig;
}


/****************************************************************************
 * @func Random linked list manipulation routines.
 */
static void
_addToFrontOfFreeList(stRegion *pregion)
{
	stSMState *state   = &smStates[pregion->bySMIndex];

	NEXT_FREE(pregion) = state->pFreeRegionList;
	state->pFreeRegionList = pregion;
}


static void
_removeFromFreeList(stRegion *pregion)
{
	stSMState *pstate   = &smStates[pregion->bySMIndex];
	stRegion  *pCurrReg = pstate->pFreeRegionList;
	stRegion  *pPrevReg = NULL;

	/* If this is the first item on the free list, just remove it */
	if (pstate->pFreeRegionList == pregion) 
	{
		pstate->pFreeRegionList = NEXT_FREE(pregion);
	}
	else 
	{
		while (pCurrReg != NULL && pCurrReg != pregion) {
			pPrevReg = pCurrReg;
			pCurrReg = NEXT_FREE(pCurrReg);
		}
	
		if (pCurrReg != NULL) {
			NEXT_FREE(pPrevReg) = NEXT_FREE(pCurrReg);
		}
	}
}


/****************************************************************************
 * @func Allocate commited virtual pages for the range covered by the
 *  region.  This routine first checks to see whether any portion of the
 *  virtual range already has a page committed to it.  If it does, (and
 *  this should only happen on the first or last page of the region, since
 *  these pages could be should with fragments from other regions), we simply
 *  increment that page's region count.  Otherwise, we allocate a new virtual
 *  page and insert it into the VPTE table.
 */
static EMUSTAT
_allocateVirtualPages(stRegion *pregion)
{
	DWORD dwIndex     = GET_FIRST_VPTE_INDEX(pregion);
	DWORD dwLastIndex = GET_LAST_VPTE_INDEX(pregion);

	for ( ; dwIndex <= dwLastIndex; dwIndex++) 
	{
		DWORD dwVPTE = GET_VPTE(pregion, dwIndex);

		if (dwVPTE == 0) {
			DWORD memhdl;

			/* Need to allocate a new page */
			dwVPTE = (DWORD) osAllocPages(1, &memhdl);
			if (dwVPTE == 0)
				RETURN_ERROR(SMERR_NO_HOST_MEMORY);

            /* Clear the memory region */
            memset((void *)dwVPTE, 0, SM_PAGE_SIZE);

			ASSERT(dwVPTE == (DWORD) memhdl);
			ASSERT(!(dwVPTE & 0xFFF));
		}

		/* Increment the region reference count */
		ASSERT(GET_VPTE_REGIONCOUNT(dwVPTE) < 0xF);
		SET_VPTE_REGIONCOUNT(dwVPTE, GET_VPTE_REGIONCOUNT(dwVPTE) + 1);
		SET_VPTE(pregion, dwIndex, dwVPTE);
	}

	pregion->byState = smstateMapped;

	return SUCCESS;
}


/****************************************************************************
 * @func Decrement the region counts for all of the pages associated with
 *  the region and, if the region count for a particular virtual page drops
 *  to zero, deallocate the page.  The name free is a little misleading,
 *  since we don't necessarily actually free the page, but from a regions
 *  point of view the page is freed.
 */
static void
_freeVirtualPages(stRegion *pregion)
{
	DWORD dwIndex     = GET_FIRST_VPTE_INDEX(pregion);
	DWORD dwLastIndex = GET_LAST_VPTE_INDEX(pregion);

	ASSERT(pregion->byFlags & RF_SMALLOCED);
	ASSERT(pregion->byState == smstateMapped);
	ASSERT(pregion);

	for ( ; dwIndex <= dwLastIndex; dwIndex++) 
	{
		DWORD dwVPTE = GET_VPTE(pregion, dwIndex);
		BYTE  byRegionCount = GET_VPTE_REGIONCOUNT(dwVPTE) - 1;

		if (byRegionCount == 0) {
			ASSERT(GET_VPTE_LOCKCOUNT(dwVPTE) == 0);
			osFreePages(dwVPTE & VPTE_PFN_MASK);
			dwVPTE = 0;
		} else {
			SET_VPTE_REGIONCOUNT(dwVPTE, byRegionCount);
		}

		SET_VPTE(pregion, dwIndex, dwVPTE);
	}

	pregion->byState = smstateAllocated;
}


/****************************************************************************
 * @func Lock all of the pages associated with a particular region.  We
 *  keep a lock count associated with each page, and we only actually call
 *  the os page lock routine when the count first transitions from 0 to 1.
 */
static EMUSTAT
_lockVirtualPages(stRegion *pregion)
{
	DWORD dwIndex     = GET_FIRST_VPTE_INDEX(pregion);
	DWORD dwLastIndex = GET_LAST_VPTE_INDEX(pregion);
	stSMState *pstate = &smStates[pregion->bySMIndex];

	/* Mapped regions are always locked, so we don't need to bother with
	 * this here.  */
	if (!(pregion->byFlags & RF_SMALLOCED))
		return SUCCESS;

	for ( ; dwIndex <= dwLastIndex; dwIndex++) 
	{
		DWORD dwVPTE = GET_VPTE(pregion, dwIndex);
		BYTE  byLockCount = GET_VPTE_LOCKCOUNT(dwVPTE);

		if (byLockCount == 0) {
			DWORD dwRetPhysAddr;
			EMUSTAT status;
			
			status = osLockVirtualRange(GET_VPTE_VADDR(dwVPTE), 1, 1, 
										&dwRetPhysAddr);
			if (status != SUCCESS)
				RETURN_ERROR(SMERR_LOCK_FAILED);

			/* Drop the page into the physical page table */
			pstate->pdwPageTable[dwIndex] = PHYSADDR_TO_PTE(dwRetPhysAddr);
		}

		/* Update the VPTE and write it back */
		ASSERT(byLockCount < 255);
		SET_VPTE_LOCKCOUNT(dwVPTE, byLockCount + 1);
		SET_VPTE(pregion, dwIndex, dwVPTE);
	}

	return SUCCESS;
}


/****************************************************************************
 * @func Unlock all of the pages associated with a particular region.  
 *  We don't actually call the os page unlock routine until the lock count
 *  in the VPTE goes to zero.
 */
static void
_unlockVirtualPages(stRegion *pregion)
{
	DWORD dwIndex     = GET_FIRST_VPTE_INDEX(pregion);
	DWORD dwLastIndex = GET_LAST_VPTE_INDEX(pregion);

	/* Mapped regions are always locked, and we don't want to mess with them */
	if (!(pregion->byFlags & RF_SMALLOCED))
		return;

	for ( ; dwIndex <= dwLastIndex; dwIndex++) 
	{
		DWORD dwVPTE = GET_VPTE(pregion, dwIndex);
		BYTE  byLockCount = GET_VPTE_LOCKCOUNT(dwVPTE);

        if (byLockCount != 0) {
            byLockCount--;

    		if (byLockCount == 0) 
	    		osUnlockVirtualRange(GET_VPTE_VADDR(dwVPTE), 1);

    		/* Update the VPTE and write it back */
	    	SET_VPTE_LOCKCOUNT(dwVPTE, byLockCount);
    		SET_VPTE(pregion, dwIndex, dwVPTE);
        }
	}
}


/****************************************************************************
 * @func Zero out the virtual page table entries associated with a region.
 *  This is useful when the entries were owned by a mapped rather than an
 *  allocated region (as is the case with streaming audio).
 *  DANGER:  This call must _NOT_ be used on regions which are not page aligned.
 *    It will destroy shared page mappings.
 */
static void
_zeroVirtualPages(stRegion *pregion)
{
	DWORD dwIndex     = GET_FIRST_VPTE_INDEX(pregion);
	DWORD dwLastIndex = GET_LAST_VPTE_INDEX(pregion);

	for ( ; dwIndex <= dwLastIndex; dwIndex++) 
	{
		SET_VPTE(pregion, dwIndex, 0x0);
	}
}

/****************************************************************************
 * @func This routine takes a voice and updates its map registers
 *  with new PTE/Index values if necessary.  We do this when we're
 *  performing a map operation, since there is no way for a playing 
 *  voice to know that the underlying mapping has changed while it
 *  is playing.  If we don't update the MAP registers, we get funny
 *  behavior, like playing old data.
 */
static void
_updateMapRegisters(stRegion *pregion, stVoice *pVoice, DWORD dwStartIndex,
                    DWORD dwNumPages, BOOL bStuff)
{
	DWORD dwMapA;
	DWORD dwMapB;
	stSMState *pstate = &smStates[pregion->bySMIndex];

    // Only update the map registers on smMemoryAddReference (bStuff=TRUE).
    // Just fix them when mapping new pages into the page table for
    // smMemoryMap (bStuff=FALSE).
    if (dwStartIndex&1)
    {
        dwMapA =  pstate->pdwPageTable[(dwStartIndex+1)%SM_NUM_PTES] & (~ 0x1FFFL);
	    dwMapA |= (dwStartIndex+1)%SM_NUM_PTES;
        dwMapB =  pstate->pdwPageTable[dwStartIndex] & (~ 0x1FFFL);
	    dwMapB |= dwStartIndex;
    }
    else
    {
        dwMapA =  pstate->pdwPageTable[dwStartIndex] & (~ 0x1FFFL);
	    dwMapA |= dwStartIndex;
        dwMapB =  pstate->pdwPageTable[dwStartIndex+1] & (~ 0x1FFFL);
	    dwMapB |= dwStartIndex+1;
    }

    if (bStuff)
    {
    	LSEPtrWrite(pVoice->halid, (MAPA | pVoice->byIndex), dwMapA);
	    LSEPtrWrite(pVoice->halid, (MAPB | pVoice->byIndex), dwMapB);
    }
    else
    {
        DWORD dwTmp;

        // First, try to fix MAPA
        dwTmp = LSEPtrRead(pVoice->halid, (MAPA | pVoice->byIndex))&0x1fffL;
        if (dwTmp == (dwMapA&0x1fffL))
    	    LSEPtrWrite(pVoice->halid, (MAPA | pVoice->byIndex), dwMapA);
        else if (dwTmp == (dwMapB&0x1fffL))
    	    LSEPtrWrite(pVoice->halid, (MAPA | pVoice->byIndex), dwMapB);
        // Then, try to fix MAPB
        dwTmp = LSEPtrRead(pVoice->halid, (MAPB | pVoice->byIndex))&0x1fffL;
        if (dwTmp == (dwMapA&0x1fffL))
    	    LSEPtrWrite(pVoice->halid, (MAPB | pVoice->byIndex), dwMapA);
        else if (dwTmp == (dwMapB&0x1fffL))
    	    LSEPtrWrite(pVoice->halid, (MAPB | pVoice->byIndex), dwMapB);
    }
}


/***************************************************************************
 * Private functions.  These are actually exported, but should only
 * be called by folks who are privy to our private header file.
 ***************************************************************************/

EMUAPIEXPORT stSMState *
smpGetSMState()
{
	return &smStates[0];
}

EMUAPIEXPORT DWORD
smpGetSMCount()
{
	return smCount;
}


/* @func smpCheckConsistency traverses the major SM data structures and
 *  checks to see whether they seem to be valid.  It returns SUCCESS if
 *  all of the data structures appear to be consistent, and a non-zero
 *  value if something is screwed.
 */
EMUAPIEXPORT EMUSTAT
smpCheckConsistency(SMID id)
{
	stSMState *pstate;
	stRegion  *pCurrReg;
	stRegion  *pNextReg;
	stRegion  *pPrevReg;
	stVoice   *pCurrVoice;
	stVoice   *pNextVoice;
	stVoice   *pPrevVoice;
    DWORD	   dwTotalMem = 0;
	DWORD	   dwFreeMem = 0;
	DWORD      dwMaxContig = 0;
	DWORD      dwNumVoices = 0;

	if ((pstate = FIND_SMSTATE(id)) == NULL)
		RETURN_ERROR(SMERR_BAD_HANDLE);
	
	/* First walk the region list and see if all of the regions are okay */
	pCurrReg = pstate->pAllRegionList;
	while (pCurrReg != NULL && dwTotalMem < SM_SAMPLE_MEMORY_SIZE) 
	{
		pPrevReg = pCurrReg->pPrevRegion;
		pNextReg = pCurrReg->pNextRegion;

		if (pPrevReg != NULL && pPrevReg->pNextRegion != pCurrReg)
			RETURN_ERROR(1);

		/* Make sure that sizes and links match up */
		if (pNextReg != NULL) {
			if (pNextReg->pPrevRegion != pCurrReg)
				RETURN_ERROR(2);

			if ((ALIGN_BASE_ADDR(pCurrReg->dwBaseAddr) + pCurrReg->dwLength) !=
				ALIGN_BASE_ADDR(pNextReg->dwBaseAddr)) 
				RETURN_ERROR(3);
		}

		if ((pCurrReg->byFlags & RF_SMALLOCED) && (pCurrReg->byState < smstateMapped))
			RETURN_ERROR(3);

		if (pCurrReg->bySMIndex != GET_SMSTINDEX(id))
			RETURN_ERROR(4);

		/* Check the voice list */
		if (pCurrReg->byState == smstateImmobile) {
			pCurrVoice  = pCurrReg->un1.pVoiceListHead;
			dwNumVoices = 0;

			while (pCurrVoice != NULL && dwNumVoices <= pCurrReg->byRefCount) 
			{
				pNextVoice = pCurrVoice->nextVoice;
				pPrevVoice = pCurrVoice->prevVoice;

				if (pPrevVoice != NULL && pPrevVoice->nextVoice != pCurrVoice)
					RETURN_ERROR(5);

				if (pNextVoice != NULL && pNextVoice->prevVoice != pCurrVoice)
					RETURN_ERROR(6);

				if (pCurrVoice->byNumRefs == 0)
					RETURN_ERROR(15);

				if (FIND_REGION(pCurrVoice->smh) != pCurrReg)
					RETURN_ERROR(16);

				pCurrVoice = pNextVoice;
				dwNumVoices++;
			}
		}

		dwTotalMem += pCurrReg->dwLength;

		if (pCurrReg->byState == smstateInvalid) {
			dwFreeMem += pCurrReg->dwLength;

			if (pCurrReg->dwLength > dwMaxContig)
				dwMaxContig = pCurrReg->dwLength;
		}


		/* Check to see if the VPTE looks correct */
		{
			DWORD dwIndex = GET_FIRST_VPTE_INDEX(pCurrReg);
			DWORD dwLastIndex = GET_LAST_VPTE_INDEX(pCurrReg);

			for ( ; dwIndex <= dwLastIndex; dwIndex++) {
				DWORD dwVPTE = GET_VPTE(pCurrReg, dwIndex);
				DWORD dwStartAddr = dwIndex * SM_PAGE_SIZE;
				DWORD dwEndAddr = dwStartAddr + SM_PAGE_SIZE;

				if ((pCurrReg->byState == smstateInvalid ||
					pCurrReg->byState == smstateAllocated)) 
				{
					if ((dwStartAddr >= ALIGN_BASE_ADDR(pCurrReg->dwBaseAddr)) &&
						(dwEndAddr <= (ALIGN_BASE_ADDR(pCurrReg->dwBaseAddr) + pCurrReg->dwLength)))
						ASSERT(dwVPTE == 0);
				} 
				else if (pCurrReg->byFlags & RF_SMALLOCED)
				{
					ASSERT(GET_VPTE_VADDR(dwVPTE) != 0);
					ASSERT(GET_VPTE_REGIONCOUNT(dwVPTE) > 0);
				}
			}
		}

		pCurrReg = pNextReg;
	}

	/* Make sure that all of our counts are accurate */
	if (dwFreeMem != pstate->dwTotalFreeMem)
		RETURN_ERROR(9);

	if (dwMaxContig != pstate->dwMaxContig)
		RETURN_ERROR(10);


	/* Check the free list for consistency */
	pCurrReg = pstate->pFreeRegionList;
	dwFreeMem = 0;
	while (pCurrReg != NULL && dwFreeMem < SM_SAMPLE_MEMORY_SIZE) {
		dwFreeMem += pCurrReg->dwLength;

		ASSERT(pCurrReg->byState == smstateInvalid);
		ASSERT(pCurrReg->byFlags == 0);

		if (pCurrReg->pNextRegion)
			ASSERT(pCurrReg->pNextRegion->pPrevRegion == pCurrReg);

		if (pCurrReg->pPrevRegion)
			ASSERT(pCurrReg->pPrevRegion->pNextRegion == pCurrReg);

		pCurrReg = NEXT_FREE(pCurrReg);
	}

	if (dwFreeMem != pstate->dwTotalFreeMem)
		RETURN_ERROR(14);

	return SUCCESS;
}



#endif /* __SM8010_C */
