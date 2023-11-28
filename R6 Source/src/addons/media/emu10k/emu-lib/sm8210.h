/******************************************************************************
*
*     Copyright (c) E-mu Systems, Inc. 1997. All rights Reserved.
*
*******************************************************************************
*
* @doc INTERNAL
* @module sm8210.h | 
* This file contains the public data types and functions used by the EMU 8010
* sample manager.  
*
* @iex 
* Revision History:
* Version	Person	Date		Reason
* -------	------	----		---------------------------------- 
*  0.002	JK		Sep 24, 97	Changed to use the 8210 name scheme.
*  0.001	JK		Sep 12, 97	Created.
******************************************************************************
* @doc EXTERNAL
* @contents1 EMU SM8010 Programmer's Manual |
*  This document describes the SM8010 sample manager API. The sample 
*  memory manager manages the E8010's page tables, which are capable of mapping
*  up to 32 MB of 16-bit samples simultaneously.  This document describes the 
*  interfaces used to allocate, free, and compact the page table memory;
*  allocate and initialize mapped host memory; and remap previously allocated 
*  memory into the E8010's address space.
*
*  The sample memory manager interracts closely with the SE8210 sound engine
*  API.  In particular, the SE8210 API requires that a sample memory handle
*  be associated with a voice so that the sound engine can locate the samples
*  during playback.  For these reasons, the reader should refer to the 
*  SE8210 Sound Engine Programmer's Guide.
*/

#ifndef __SM8210_H
#define __SM8210_H

/*****************************************************************************
 * Includes
 *****************************************************************************/

#include "datatype.h" 
#include "aset8210.h"
#include "se8210.h"		/* Need definition for SEVOICE */

/*****************************************************************************
 * Defines
 *****************************************************************************/

#define SMERR_BAD_HANDLE         1
#define SMERR_INIT_FAILED        2
#define SMERR_NO_SAMPLE_MEMORY   3
#define SMERR_NO_HOST_MEMORY     4
#define SMERR_BAD_ADDRESS        5
#define SMERR_NO_MAPPING         6
#define SMERR_BAD_VOICE			 7
#define SMERR_OUT_OF_BOUNDS		 8
#define SMERR_ALREADY_DISCOVERED 9
#define SMERR_SAMPLE_MEMORY_FRAGMENTED 10
#define SMERR_CORRUPT			 11
#define SMERR_MAPPING_FAILED     12
#define SMERR_LOCK_FAILED		 13

#define SM_PAGE_SIZE		4096	/* The page size of the Emu chip */
#define SM_NUM_PTES			8192	/* Number of page table entries on the
												 * E8010 */

/* smMemoryAllocate flags */
#define SMAF_ALIGN_MASK         0x1f    /* For sample alignment */
#define SMAF_ALLOC_HOST			0x20	/* Allocate and map host pages */
#define SMAF_8BIT_DATA			0x40	/* The sample data is 8-bit data
										 * and thus should be allocated in
										 * the first 16MB of SM8010 memory */
#define SMAF_LOCK				0x80	/* Lock the pages into memory */

/* smMemoryMap flags */
#define SMMAP_VIRTUAL			0x2     /* Map virtual memory */
#define SMMAP_PHYSICAL			0x1     /* Map physical pages */
#define SMMAP_LOCK              0x4	    /* Indicates that virtual address range
                                         * needs to be locked  */

BEGINEMUCTYPE

/*****************************************************************************
 * Enumerations
 *****************************************************************************/

/* @enum SMSTATEVALUE | This enumerated type lists all of the states in which
 *  an SMHANDLE can exist.  Although these states are mostly used internally
 *  by SM8210 for book-keeping purposes, a user can query the current state
 *  with the <f smMemoryGetStatus> function.
 */
typedef enum enSMStateTag {
	smstateInvalid,	/* @emem indicates that the given region is 
							 *  invalid and available for allocation. */
	smstateAllocated,	/* @emem Indicates that the region has reserved
							 *  some portion of the E8010 address space, but
							 *  that no particular page table entries have
							 *  been allocated.  */
	smstateMapped,		/* @emem Indicates that actual page table entries
							 *  have been allocated, but the sound engine
							 *  is not currently referencing them.  A region
							 *  in this state can be compacted.  */
	smstateImmobile	/* @emem Indicates that page table entries have
							 *  been allocated and that the sound engine
							 *  is currently (or will soon be) DMA'ing data
							 *  from the region. The page table entries are 
							 *  locked and can be neither compacted nor 
							 *  deallocated.
							 */
} SMSTATEVALUE;


/* @enum SMORIGINVALUE | Contains the set of valid origin values used
 *  by <f smMemorySetPointer>.
 */
typedef enum smenOriginValueTag 
{
	smoriginBegin,		/* @emem The offset is relative to the beginning of the
						 *  region   */
	smoriginCurrent,	/* @emem The offset is relative to the logical pointer's
						 *  current location.  */
	smoriginEnd			/* @emem The offset is relative to the end of the 
					     *  region */
} SMORIGINVALUE;


/*****************************************************************************
 * Structures
 *****************************************************************************/

/* typedef DWORD SEVOICE; */

/* @type SMID | An SMID is an opaque handle which is used to refer to a 
 *  particular instance of an Sample Memory Manager.  Each discovered E8010 
 *  chip in a system has a corresponding Sample Memory Manager, and that 
 *  Sample Memory Manager is addressed through an SMID.
 */
typedef DWORD SMID;


/* @type SMHANDLE | An SMHANDLE is an opaque handle which refers to an
 *  allocated region of E8010 sample memory.  If this portion of E8010
 *  memory is mapped to host memory, the SMHANDLE can also be used to reference 
 *  the physical addresses of the mapped pages in host memory.
 *  The SMHANDLE is used by the majority of the SM8210's functions.
 */
typedef DWORD SMHANDLE;


/* @struct SMATTRIB | An attribute structure. XXX Don't know what goes in
 *  in here yet.
 */
typedef struct stsmAttribTag {
	DWORD  dwRandom;		/* @field A random placeholder */
} SMATTRIB;


/* @struct SMHANDLEINFO | The SMHANDLEINFO contains useful information
 *  concerning the current state of an SMHANDLE.  A user can fill
 *  an SMHANDLEINFO structure by calling the <f Whatever> function.
 */
typedef struct stsmHandleInfoTag {
    SMSTATEVALUE state;		/* @field The current state of the region. */
	DWORD	dwBaseAddr;		/* @field The E8010 base address of the region. */
	DWORD	dwSize;			/* @field The size of the region in 4K pages */
	DWORD	dwNumVoices;	/* @field The number of references to the region */
	DWORD	voices[64];		/* @field The voices referencing this handle */
} SMHANDLEINFO;


/* @struct SMMAP | The SMMAP structure is used to pass page mapping  
 *  information to the sample memory manager.
 */
typedef struct stsmMapTag {
	DWORD	dwFlags;		/* @field A set of flags describing the mapping. 
							 *  The following flags are valid:
							 *  @flag SMMAP_VIRTUAL | When this flag is set,
							 *   <t pBaseAddr> contains the virtual address
							 *   of the first byte to be mapped, and 
							 *   <t dwLength> contains the length (in bytes)
							 *   of the virtual mapping.
							 *  @flag SMMAP_PHYSICAL | When this flag is set,
							 *   <t dwLength> contains the length of the
							 *   mapping in bytes, and <t pdwPhysicalPageList>
							 *   points to an array containing
							 *   a list of the host's physical memory pages.
							 * @end
							 * Note that SMMAP_VIRTUAL and SMMAP_PHYSICAL are
							 * mutually exclusive.   */
	BYTE *	pBaseAddr;		/* @field When setting up a virtual mapping, this
							 *  field contains the base virtual address of the
							 *  mapping.  This virtual address range must be
							 *  locked (see <f osLockVirtualRange>) before calling
							 *  <f smMemoryMap>.  If it is ever unlocked, the
							 *  mapping must be refreshed by relocking the range
							 *  and calling <f smMemoryMap> again.  When setting
							 *  up a physical mapping, this should contain the
							 *  virtual (linear) address of the first physical
							 *  page in the list */
	DWORD   dwOffset;		/* @field The offset in the region at which the 
							 *  mapping is to begin.  */
	DWORD	dwLength;	    /* @field For virtual mappings, this contains 
							 *  the number of bytes in the mapping.  For 
							 *  physical mappings, this contains the number
							 *  of pages in the page list.  */
    DWORD * pdwPhysicalPageList; /* @field For physical mappings, this points to
							 *  a list of host physical addresses.  
							 */
} SMMAP;


/*****************************************************************************
 * Functions
 *****************************************************************************/

/* @func This function fills an array with the SMIDs of all of
 *  the discovered sample memory managers in the system and returns a 
 *  count of the total number of managers.  The caller is allowed to
 *  pass NULL for either or both of the arguments; in this case,
 *  the function will just return the total number of sample managers
 *  without attempting to dereference the array.
 *
 *  This function only returns SMIDs for discovered instances of the
 *  Sample Memory Manager.
 *
 * @parm DWORD | count | The number of SMID handles in the array.
 *  If 'count' is less than the total number of SM8210 instances, only
 *  the first 'count' SMIDs will be copied into the array.  If 'count' is 0,
 *  the function will not attempt to fill the array.
 * @parm SMID * | seids | An array of 'count' SEID handles.
 *	If NULL, the routine will not attempt to fill the array with IDs.
 * 
 * @rdesc The total number of SM8210 instances in the system.  If an error
 *  occurs, the function will return 0.
 */
EMUAPIEXPORT DWORD smGetHardwareInstances(DWORD count /* VSIZE */, SMID *smids /* IO */);


/* @func Copies the name of the specified SM8210 instance into the given
 *  array.  At most 'count' characters are copied.  The system guarantees
 *  that an SM8210 name will always be fewer than 63 characters in
 *  length, so a caller can safely allocate the space for the string on the
 *  stack.
 *
 * @parm SMID | smid | The ID of the sample memory manager whose name is being
 *  retrieved.
 * @parm DWORD | count | The size of the destination array, in characters.
 *  If the 'count' is less than the length of the name string, 'count' - 1 
 *  characters will be copied into the array and a terminating '\0' will be 
 *  placed in the last element of the array.
 * @parm char * | szName | A character array into which the device name will
 *  copied.
 *
 * @rdesc Returns SUCCESS if the retrieval completed successfully.  Otherwise,
 *  one of the following error values is returned:
 *		@flag SMERR_BAD_HANDLE | the sound engine corresponding to seid wasn't 
 *		 found.  This shouldn't happen unless an invalid seid is passed.
 *		@flag SMERR_BAD_PARAM | A NULL or invalid pointer was passed in for 
 *		 szName.
 */
EMUAPIEXPORT EMUSTAT smGetModuleName(SMID smid, DWORD count /* VSIZE */, 
									 char *szName /* IO */);


/* @func Fills an attribute data structure with the attributes for the
 *  specified sample manager.
 */
EMUAPIEXPORT EMUSTAT smGetModuleAttributes(SMID id, SMATTRIB *attrib /* IO */);


/* @func Retrieves the total amount of unallocated E8010 sample memory 
 *  and the size of the largest contiguous block of sample memory. 
 *  Both values are in unit of bytes.
 * 
 * @parm SMID | smid | The ID of the sample memory manager to be queried.
 * @parm DWORD * | retTotalFree | A pointer to the DWORD in which the
 *  total amount of free sample memory (in bytes) is to be stored.
 * @parm DWORD * | retMaxContig | A pointer to the DWORD in which the
 *  size of the largest contigous block of sample memory (in bytes) is to be 
 *  stored.
 *
 * @rdesc If the function completes successfully, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE | The SMID passed is invalid.
 *	@flag SMERR_BAD_ADDRESS | Either the retTotalFree or retMaxConfig
 *   pointer is invalid.
 */
EMUAPIEXPORT EMUSTAT smGetSampleMemoryStatus(SMID smid, DWORD *retTotalFree,
								 DWORD *retMaxConfig);


/* @func Compacts the E8010 sample memory of the device corresponding
 *  to the SMID.  During the compaction process, the SM8210 attempts
 *  to collect all of the fragmented pieces of unallocated sample memory
 *  into a single contiguous block.  Since regions in the IMMOBILE state
 *  can't be moved, the compaction process may not be able to completely
 *  compact the sample memory.
 *
 * @parm SMID | smid | The ID of the sample memory manager.
 * 
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE | The given SMID is invalid.
 */
EMUAPIEXPORT EMUSTAT smCompactSampleMemory(SMID smid);
  

/* @func Allocates a contiguous region of EMU8010 sample memory and,
 *  optionally, and backing host physical memory if requested.  If
 *  the client specifies the SMAF_ALLOC_HOST flag, <f smMemoryAllocate>
 *  will perform the allocation of the host memory and set up the mapping
 *  between the sample memory and the host physical memory automatically.
 *  Otherwise, this function will allocate only the sample memory, and
 *  the client will have to explicitly establish the mapping with the
 *  <f smMemoryMap> function.  If the client elects to perform the 
 *  mapping itself, it is responsible for providing <f smMemoryMap> with
 *  a list of properly allocated and locked physical pages.
 *  A client can also specify the SMAF_8BIT_DATA flag, which indicates
 *  that the region will be used for 8 bit samples and thus should be
 *  allocated in the first 16 Megabytes of E8010's sample memory.
 *
 * @parm SMID  | smid   | The sample memory manager which will service the
 *  allocation request.
 * @parm DWORD | dwSize | The size (in bytes) of the region to allocate. 
 *  This size will automatically be rounded up to an integral multiple of
 *  of the EMU8010's page size.
 * @parm DWORD | dwFlags | A set of allocation flags.
 *	@flag SMAF_ALLOC_HOST | If specified, the function will allocate
 *   sufficient pages from the host's memory to completely map the
 *   region.  If this flag isn't used, the client will be responsible
 *   for allocating and mapping the host memory.
 *	@flag SMAF_8BIT_DATA | The region will contain 8 bit samples and needs
 *   to be placed in the first 16MB of sample memory.
 * @parm SMHANDLE * | retHandle | The returned handle to the newly allocated
 *  region.
 *  
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_ADDRESS | The SMHANDLE pointer is invalid.
 *  @flag SMERR_NO_SAMPLE_MEMORY | Couldn't find a block of E8010 sample
 *   memory that was large enough to satisfy the request.
 *  @flag SMERR_NO_HOST_MEMORY | Couldn't allocate enough host memory
 *   to map the entire allocated range.  This error can only occur if
 *   the SMAF_ALLOC_HOST flag is set.
 *  @flag SMERR_SAMPLE_MEMORY_FRAGMENTED | There is enough total free memory
 *   to satisfy this request, but no single contiguous region of sample memory
 *   is large enough to satisfy the request.  The caller can call 
 *   <f smCompactSampleMemory> to try and create larger contiguous regions.
 *  @flag SMERR_CORRUPT | The sample memory manager's data structures are
 *   internally inconsistent.  This error value generally indicates that there
 *   is a bug in the sample memory manager somewhere.
 */
EMUAPIEXPORT EMUSTAT smMemoryAllocate(SMID smid, DWORD dwSize, DWORD dwFlags, 
									  SMHANDLE *retHandle /* IO */);


/* @func Frees the E8010 sample memory associated with the handle.  If
 *  the region was originally allocated with the SMAF_ALLOC_HOST flag
 *  set, this function will also free the host memory associated with
 *  the region.
 *
 * @parm SMHANDLE | smh | The handle for the region to free.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 */
EMUAPIEXPORT EMUSTAT smMemoryFree(SMHANDLE smh);


/* @func Adds a single voice reference to the specified sample memory
 *  handle.  If the memory was previously unreferenced, it will be
 *  committed to the IMMOBILE state and all of the sample memory pages
 *  associated with the region will be locked.  Each subsequent reference will
 *  then just increment the reference count.  This function is called
 *  automatically by the sound engine startup functions <f seVoiceStart>
 *  and <f seVoiceSetupAndStart>, but a client can call it directly if
 *  necessary.  
 *
 * @parm SMHANDLE | smh | The sample memory handle to reference.
 * @parm SEVOICE  | voice | The SEVOICE which is referencing the voice.
 *  A value of 0 can be passed if no particular voice is referencing the
 *  region and the caller just wants to insure that the region is locked.
 *  When this is done, however, the caller must call <f smMemoryRemoveReference>
 *  before freeing the voice.
 * 
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_VOICE   | The given SEVOICE handle is invalid.
 */
EMUAPIEXPORT EMUSTAT  smMemoryAddReference(SMHANDLE smh, DWORD voice);


/* @func Removes a reference from the specified sample memory handle.
 *  If the reference count drops to zero, the region's state will
 *  change from IMMOBILE to ALLOCATED.  See the description of the
 *  SMSTATEVALUE enumerated type for more information on the connotations
 *  of different states.
 *
 * @parm SMHANDLE | smh | The sample memory handle to unreference.
 * @parm SEVOICE  | sevoice | The handle of the voice which is no
 *  longer referencing the memory.  If the voice was originally referenced
 *  via an explicit call to smMemoryAddReference with an SEVOICE of 0, this
 *  function may also be called with an SEVOICE of 0 to remove the reference.
 *  It is a bad idea, however, to use a value of 0 to remove references which
 *  weren't originally added by the caller, since this may corrupt the 
 *  referencing voice list.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_VOICE   | The given SEVOICE handle is invalid, or
 *   the voice isn't actually referencing the region.
 */
EMUAPIEXPORT EMUSTAT  smMemoryRemoveReference(SMHANDLE smh, SEVOICE voice);


/* @func Retrieves the current status of the given SMHANDLE.  The
 *  meanings of the various handle states are described in the
 *  SMSTATEVALUE enumerated type.
 *
 * @parm SMHANDLE | smh | The Sample Memory Handle to query.
 * @parm SMSTATEVALUE * | retStatus | Returned state.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE | The given SMHANDLE is invalid.*
 */
EMUAPIEXPORT EMUSTAT smMemoryGetStatus(SMHANDLE smh, SMSTATEVALUE *retStatus);


/* @func Fills in an SMHANDLEINFO structure with data describing the current
 *  state of the Sample Memory Handle.  See the description of the 
 *  <t SMHANDLEINFO> structure for more details.
 *
 * @parm SMHANDLE | smh | The Sample Memory Handle to examine.
 * @parm SMHANDLEINFO * | retInfo | The SMHANDLEINFO structure to 
 *  initialize.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_ADDRESS | The SMHANDLEINFO pointer is invalid.
 */
EMUAPIEXPORT EMUSTAT smMemoryGetInfo(SMHANDLE smh, SMHANDLEINFO *retInfo);


/* @func Get the base and length of the region referenced by the
 *  given SMHANDLE.  While this information is available through
 *  the <f smMemoryGetInfo> function, this approach is slightly 
 *  faster if all the client needs is the base and length.
 */
EMUAPIEXPORT EMUSTAT smMemoryGetBaseAndLength(SMHANDLE smh, DWORD *retBaseAddr /* IO */,
											  DWORD *retLength /* IO */);


/* @func Fills in an SMMAP structure with the current physical address
 *  mapping used by the specified SMHANDLE.  In order to retrieve the
 *  host physical address array, the client must first allocate an
 *  array of DWORDS with sufficient space to accomodate the number of
 *  physical page addresses desired.  The number of bytes that the caller
 *  is interested in should be written into the <t dwLength> field of the 
 *  SMMAP structure, and the pointer to the allocated array should be copied 
 *  to the <t pdwPhysicalPageList> field.  Upon return, the length of the 
 *  mapping (in bytes) will be returned <t dwLength>, the host virtual
 *  address for the mapping will be in <t pBaseAddr>, and a list of physical
 *  page addresses will be written into the <t retMap> array.
 *
 * @parm SMHANDLE | smh | The Sample Memory Handle whose map is to be 
 *  retrieved.
 * @parm SMMAP *  | retMap | Pointer to the map to initialize.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_ADDRESS | The SMMAP pointer is invalid.
 *  @flag SMERR_NO_MAPPING  | The region associated with SMHANDLE has
 *   not yet been mapped.
 */
EMUAPIEXPORT EMUSTAT smMemoryGetMap(SMHANDLE smh, SMMAP *retMap);

EMUAPIEXPORT EMUSTAT smUpdateMapRegisters(SMHANDLE smh, BYTE byVoiceIndex,
                                          DWORD dwCurPageIndex,
                                          DWORD dwNextPageIndex);

/* PP smMemoryGetMap3.inc, smMemoryGetMap0.inc */


/* @func Reads data out of the region specified by the SMHANDLE and
 *  copies it into into the buffer. This function requires that the
 *  region either be in the MAPPED state or that the region was originally
 *  allocated using the SMAF_ALLOC_HOST flag.
 *  The transfer begins at <t dwOffset> bytes from the beginning of the region,
 *  and <t dwCount> bytes are copied from the region into the buffer indicated
 *  by <t pbyBuffer>.
 *  
 * @parm SMHANDLE | smh | The sample memory handle to read from.
 * @parm BYTE *   | byBuffer | A pointer to the buffer that receives
 *	the data read from the memory region.
 * @parm DWORD    | dwOffset | The offset from the beginning of the region
 *  to begin copying from.
 * @parm DWORD    | dwCount  | The number of bytes to be read from the region.
 *  the number of bytes read.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_ADDRESS | The byBuffer pointer is invalid.
 *  @flag SMERR_NO_MAPPING  | The region has no host memory mapped to it.
 *  @flag SMERR_END_OF_REGION | An attempt to read past the end of the region
 *   was made.
 */
EMUAPIEXPORT EMUSTAT smMemoryRead(SMHANDLE smh, BYTE *pbyBuffer,  DWORD dwOffset, 
								  DWORD dwCount);
/* PP smMemoryRead3.inc, smMemoryRead0.inc */


/* @func Writes data from the given buffer into the region specified
 *  by SMHANDLE.  This function requires that the region either be in
 *  MAPPED state or that the region was originally allocated using the
 *  SMAF_ALLOC_HOST flag.  Write begins writing data to the region at the
 *  at <t dwOffset> bytes from the beginning of the region, and <t dwCount>
 *  bytes are copied from the <t byBuffer> into the region indicated by
 *  <t smh>.
 *
 * @parm SMHANDLE | smh      | The sample memory handle to write to.
 * @parm BYTE *   | byBuffer | A pointer to the buffer containing the data
 *  to be written to the region.
 * @parm DWORD    | dwOffset | The offset from the beginning of the region
 *  where copying begins.
 * @parm DWORD    | dwCount  | The number of bytes to be written to the region.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_ADDRESS | The byBuffer pointer is invalid.
 *  @flag SMERR_NO_MAPPING  | The region has no host memory mapped to it.
 *  @flag SMERR_END_OF_REGION | An attempt to read past the end of the region
 *   was made.
 */
EMUAPIEXPORT EMUSTAT smMemoryWrite(SMHANDLE smh, BYTE *byBuffer, 
								   DWORD dwOffset, DWORD dwCount);
/* PP smMemoryWrite3.inc, smMemoryWrite0.inc */


/* @func Establishes a mapping between a region of E8010 sample memory
 *  and a set of physical pages in the host's memory.  This function
 *  may be used to map or remap the entire region or any portion of it.
 *  The mappings may be modified at any time, including when the region
 *  is IMMOBILE.  The client must be very careful to ensure that the 
 *  sound engine is never attempting to play from sample memory which
 *  doesn't have a valid mapping.
 * 
 * @parm SMHANDLE | smh | The handle to the region for which the mapping
 *  will be established.
 * @parm SMMAP *  | smmap | A pointer to the smmap structure which contains
 *  a description of the memory range to be mapped.
 *
 * @rdesc If the function executes without error, SUCCESS is returned.
 *  Otherwise, one of the following error codes is returned:
 *	@flag SMERR_BAD_HANDLE  | The given SMHANDLE is invalid.
 *  @flag SMERR_BAD_ADDRESS | The map pointer is invalid.
 */
EMUAPIEXPORT EMUSTAT smMemoryMap(SMHANDLE smh, SMMAP *smmap);

ENDEMUCTYPE

#endif

