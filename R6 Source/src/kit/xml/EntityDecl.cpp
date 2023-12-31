/* :ts=8 bk=0
 *
 * mem.h:	Definitions for internal memory management.
 *
 * Leo L. Schwab					1999.01.04
 */
#ifndef _MEM_H
#define	_MEM_H

#ifndef _OS_H
#include <kernel/OS.h>
#endif
#ifndef _LISTNODE_H
#include <dinky/listnode.h>
#endif
#ifndef _BENA4_H
#include <dinky/bena4.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Each region of free space is represented by a MemNode.
 */
typedef struct MemNode {
	struct Node	mem_Node;
	uint32		mem_Addr;
	uint32		mem_Size;
} MemNode;

/*
 * The entire allocatable resource (e.g. all memory on the graphics card)
 * is described by a MemList.  Individual MemNodes attached to the MemList
 * describe unallocated space.
 */
typedef struct MemList {
	struct List	ml_List;
	struct Bena4	ml_Lock;	/*  Thread arbitration     */
	uint32		ml_BaseAddr;	/*  Base of entire region  */
	uint32		ml_Size;	/*  Size "    "      "     */
	const char	*ml_Name;	/*  Maybe helpful someday  */
	void		*(*ml_allocfunc)(void);    /*  Called to alloc and  */
	void		(*ml_freefunc)(void *addr);/*   free MemNodes  */
} MemList;


extern status_t AllocMem (struct BMemSpec *ms, struct MemList *ml);
extern status_t FreeMem (struct BMemSpec *ms, struct MemList *ml);
extern status_t InitMemList (struct MemList *ml);
extern void DisposeMemList (struct MemList *ml);
extern uint32 align_up (uint32 base_orig, uint32 carebits, uint32 statebits);
extern uint32 align_down (uint32 base_orig, uint32 carebits, uint32 statebits);


#ifdef __cplusplus
}
#endif

#endif	/*  _MEM_H  */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                          /* :ts=8 bk=0
 *
 * mempool.c:	MemPool manager.
 *
 * Leo L. Schwab					1999.03.14
 */
#include <kernel/OS.h>
#include <drivers/KernelExport.h>
#include <drivers/bus_manager.h>
#include <dinky/bena4.h>
#include <dinky/listnode.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <surface/mempool.h>
#include <drivers/mempool_module.h>

#include "mem.h"


/****************************************************************************
 * #defines
 */
#define	NNODES		512		/*  FIXME: Change to dynamic alloc  */


/*****************************************************************************
 * Bookkeeppiinngg structures.
 */
typedef struct memlayout {
	Node		ml_node;	/*  I wuv wists!		*/
	Node		ml_ownernode;
	BMemLayoutDesc	ml_mld;
	uint32		ml_opencount;
} memlayout;

typedef struct mempool {
	Node		mp_node;	/*  Oh, look, another nail...	*/
	Node		mp_ownernode;	/*  Who created this		*/
	BMemPoolDesc	mp_mpd;
	MemList		*mp_memlist;
} mempool;


/****************************************************************************
 * Prototypes.
 */
static status_t initdefaultallocator (struct mempool *m);


/****************************************************************************
 * Globals.
 */
static List	gMemPools;
static List	gLayoutList;
static Bena4	gLock;
static int32	gMaxLayoutID = -1;
static int32	gMaxPoolID = -1;

static MemNode	gnodes[NNODES];
static uint32	gfirstfree;


/****************************************************************************
 * Memory pool management.
 */
static status_t
register_mempool (struct BMemPoolDesc *desc)
{
	mempool *mp;

	if (!(mp = malloc (sizeof (mempool))))
		return (B_NO_MEMORY);
	memset (mp, 0, sizeof(*mp));
	memcpy (&(mp->mp_mpd), desc, sizeof (*desc));
	
	
	if (!mp->mp_mpd.mpd_CallBacks) {
		status_t retval;

		if ((retval = initdefaultallocator (mp)) < 0) {
			free (mp);
			return (retval);
		}
	} else
		mp->mp_memlist = NULL;

	/*
	 * Everything went okay.  Add it to the global list and tell the client
	 * 