ena4 (&gLock);
	mp->mp_mpd.mpd_PoolID = ++gMaxPoolID;
	AddTail ((Node *) mp, &gMemPools);
	BUnlockBena4 (&gLock);

	desc->mpd_PoolID = mp->mp_mpd.mpd_PoolID;
	return (B_OK);
}

static status_t
unregister_mempool (struct BMemPoolDesc *desc)
{
	mempool	*mp;

	BLockBena4 (&gLock);

	for (mp = (mempool *) FIRSTNODE (&gMemPools);
	     NEXTNODE (mp);
	     mp = (mempool *) NEXTNODE (mp))
	{
		if (mp->mp_mpd.mpd_PoolID == desc->mpd_PoolID) {
			/*  Found the pool s/he wants to get rid of.  */
			RemNode ((Node *) mp);

			if (mp->mp_memlist) {
				DisposeMemList (mp->mp_memlist);
				free (mp->mp_memlist);
			}
			free (mp);
			BUnlockBena4 (&gLock);
			return (B_OK);			
		}
	}

	BUnlockBena4 (&gLock);
	return (B_BAD_VALUE);
}

static status_t
getmempooldesc (struct BMemPoolDesc *desc, int type)
{
	mempool		*mp;
	status_t	retval = B_BAD_VALUE;

	BLockBena4 (&gLock);

	for (mp = (mempool *) FIRSTNODE (&gMemPools);
	     NEXTNODE (mp);
	     mp = (mempool *) NEXTNODE (mp))
	{
		if ((type == 0  &&
		     mp->mp_mpd.mpd_PoolID == desc->mpd_PoolID)  ||
		    (type == 1  &&
		     !strcasecmp (mp->mp_mpd.mpd_Name, desc->mpd_Name)))
		{
			memcpy (desc, &mp->mp_mpd, sizeof (*desc));
			retval = B_OK;
			break;
		}
	}

	BUnlockBena4 (&gLock);
	return (retval);
}

static status_t
getmempooldesc_byid (struct BMemPoolDesc *desc)
{
	return (getmempooldesc (desc, 0));
}

static status_t
getmempooldesc_byname (struct BMemPoolDesc *desc)
{
	return (getmempooldesc (desc, 1));
}


/****************************************************************************
 * Memory allocator/deallocator.
 */
/*  Allocator call-backs.  */
static void *
allocnode (void)
{
	register int	i;
	
	for (i = gfirstfree;  i < NNODES;  i++)
		if (ISORPHANNODE (gnodes + i)) {
			gfirstfree = i + 1;
			return (gnodes + i);
		}
	return (NULL);
}

static void
freenode (void *node)
{
	register int32	idx;

	/*  Client is presumed to have called RemNode()  */
	if ((idx = (MemNode *) node - gnodes) < gfirstfree)
		gfirstfree = idx;
}


static status_t
initdefaultallocator (struct mempool *mp)
{
	MemList		*ml;
	status_t	retval;

	if (!(ml = malloc (sizeof (MemList))))
		return (B_NO_MEMORY);

	ml->ml_BaseAddr	= mp->mp_mpd.mpd_BaseAddr;
	ml->ml_Size	= mp->mp_mpd.mpd_Size;
	ml->ml_Name	= mp->mp_mpd.mpd_Name;
	ml->ml_allocfunc= allocnode;
	ml->ml_freefunc = freenode;
	if ((retval = InitMemList (ml)) < 0) {
		free (ml);
		return (retval);
	}
	
	mp->mp_memlist = ml;
	mp->mp_mpd.mpd_CallBacks = NULL;

	return (B_OK);
}


static status_t
memalloc (struct BMemSpec *spec)
{
	status_t	retval;

	BLockBena4 (&gLock);

	/*
	 * FIXME: Handle MEMPOOL_ANYPOOL
	 * FIXME: Define errors for bad MemPool ID and non-default allocator.
	 */
	if (spec->ms_PoolID == 1) {
		// do allocation for system RAM
		// ISSUE: Should we proclaim that system RAM allocations be
		// done by the client?
		// ISSUE: Shouldn't, in the grand scheme of things, all this
		// mish-mash be a part of the kernel?
	} else {
		register mempool *mp;

		for (mp = (mempool *) FIRSTNODE (&gMemPools);
		     NEXTNODE (mp);
		     mp = (mempool *) NEXTNODE (mp))
		{
			if (mp->mp_mpd.mpd_PoolID == spec->ms_PoolID) {
				BMemPoolCallBacks	*mpcb;
				
				if (mpcb = mp->mp_mpd.mpd_CallBacks)
					retval = (mpcb->mpcb_Alloc)
						  (spec, mpcb->mpcb_Cookie);
				else if (mp->mp_memlist)
					retval =
					 AllocMem (spec, mp->mp_memlist);
				else
					/*  This ought to get my attention  */
					retval = B_BUSTED_PIPE;
				break;
			}
		}
	}

	BUnlockBena4 (&gLock);
	return (retval);
}

static status_t
memfree (struct BMemSpec *spec)
{
	status_t	retval;

	BLockBena4 (&gLock);

	if (spec->ms_PoolID == 1) {
		// do free of system RAM
	} else {
		register mempool *mp;

		for (mp = (mempool *) FIRSTNODE (&gMemPools);
		     NEXTNODE (mp);
		     mp = (mempool *) NEXTNODE (mp))
		{
			if (mp->mp_mpd.mpd_PoolID == spec->ms_PoolID) {
				BMemPoolCallBacks	*mpcb;
				
				if (mpcb = mp->mp_mpd.mpd_CallBacks)
					retval = (mpcb->mpcb_Free)
						  (spec, mpcb->mpcb_Cookie);
				else if (mp->mp_memlist)
					retval =
					 FreeMem (spec, mp->mp_memlist);
				else
					retval = B_BUSTED_PIPE;
				break;
			}
		}
	}

	BUnlockBena4 (&gLock);
	return (retval);
}


/****************************************************************************
 * MemLayout management.
 */
static status_t
obtainlayoutid (struct BMemLayoutDesc *desc, bool createnew)
{
	memlayout	*ml;

	BLockBena4 (&gLock);

	/*
	 * Search for matching existing layout name.
	 */
	for (ml = (memlayout *) FIRSTNODE (&gLayoutList);
	     NEXTNODE (ml);
	     ml = (memlayout *) NEXTNODE (ml))
	{
		if (!strcasecmp (desc->mld_DataType,
				 ml->ml_mld.mld_DataType)  &&
		    !strcasecmp (desc->mld_DataLayout,
		    		 ml->ml_mld.mld_DataLayout))
			goto returnid;	/*  Look down  */
	}
	if (!createnew) {
		BUnlockBena4 (&gLock);
		return (B_NAME_NOT_FOUND);
	}

	/*
	 * No matching names.  Create a new one.
	 */
	if (!(ml = malloc (sizeof (*ml)))) {
		BUnlockBena4 (&gLock);
		return (B_NO_MEMORY);
	}

	memcpy (&ml->ml_mld, desc, sizeof (*desc));
	ml->ml_mld.mld_LayoutID	= ++gMaxLayoutID;
	ml->ml_opencount	= 0;

	/*  Install into global list.  */
	AddTail ((Node *) ml, &gLayoutList);

returnid:
	ml->ml_opencount++;
	BUnlockBena4 (&gLock);
	desc->mld_LayoutID = ml->ml_mld.mld_LayoutID;
	return (B_OK);
}


static status_t
releaselayoutid (struct BMemLayoutDesc *desc)
{
	memlayout	*ml;
	status_t	retval = B_BAD_VALUE;
	uint32		id;

	id = desc->mld_LayoutID;
	BLockBena4 (&gLock);

	for (ml = (memlayout *) FIRSTNODE (&gLayoutList);
	     NEXTNODE (ml);
	     ml = (memlayout *) NEXTNODE (ml))
	{
		if (id == ml->ml_mld.mld_LayoutID) {
			if (--ml->ml_opencount == 0) {
				RemNode ((Node *) ml);
				free (ml);
			}
			retval = B_OK;
			break;	/*  MUST do this, or the loop may lock  */
		}
	}
	BUnlockBena4 (&gLock);
	
	return (retval);
}


static status_t
getlayoutdesc (struct BMemLayoutDesc *desc)
{
	memlayout	*ml;
	status_t	retval = B_BAD_VALUE;

	BLockBena4 (&gLock);

	for (ml = (memlayout *) FIRSTNODE (&gLayoutList);
	     NEXTNODE (ml);
	     ml = (memlayout *) NEXTNODE (ml))
	{
		if (desc->mld_LayoutID == ml->ml_mld.mld_LayoutID) {
			memcpy (desc, &ml->ml_mld, sizeof (*desc));
			retval = B_OK;
			break;
		}
	}
	BUnlockBena4 (&gLock);
	return (retval);
}


/*****************************************************************************
 * Module setup/teardown.
 */
status_t
init (void)
{
	status_t retval;

	if ((retval = BInitOwnedBena4 (&gLock,
				       "MemPool module list lock",
				       B_SYSTEM_TEAM)) == B_OK)
	{
		register int	i;
		
		for (i = 0;  i < NNODES;  i++)
			INITNODE (&gnodes[i].mem_Node);

		InitList (&gMemPools);
		InitList (&gLayoutList);
		gMaxLayoutID = 0;
		gMaxPoolID = 1;		/*  1 == system RAM  */
		
		/*  FIXME: (?)  Create default descriptor for system RAM  */
	}

dprintf ("||| init: returning %d\n", retval);
	return (retval);
}

static status_t
uninit (void)
{
	/*
	 * Gee, I hope no one's still using these Layouts and Pools, since
	 * all record of them is about to go away.
	 */
	if (gMaxLayoutID < 0)
		/*  Never initialized; we're outa there.  */
		return (B_OK);

	/*  Tear down MemLayouts.  */
      {	memlayout	*ml;

	while (ml = (memlayout *) RemHead (&gLayoutList)) {
#if DEBUG
		if (ml->ml_opencount)
			dprintf (("+++memlayout: WARNING: memlayout %d (%s/%s) logged as still in use.\n",
				ml->ml_mld.mld_LayoutID,
				ml->ml_mld.mld_DataType,
				ml->ml_mld.mld_DataLayout));
#endif
		free (ml);

	}
      }

	/*  Tear down MemPools.  */
      {	mempool		*mp;

	while (mp = (mempool *) RemHead (&gMemPools)) {
		if (mp->mp_memlist) {
			DisposeMemList (mp->mp_memlist);
			free (mp->mp_memlist);
		}
		free (mp);
	}
      }

	BDisposeBena4 (&gLock);
	gMaxLayoutID = -1;
	
	return (B_OK);
}

static status_t
std_ops (int32 op, ...)
{
	switch (op) {
	case B_MODULE_INIT:
		return (init ());
	case B_MODULE_UNINIT:
		return (uninit ());
	default:
		return (B_ERROR);
	}
}

static status_t
rescan (void)
{
	return (B_OK);
}





static mempool_module	mpmod = {
	{
		B_MEMPOOL_MODULE_NAME,
		B_KEEP_LOADED,
		std_ops
	},
	register_mempool,	/* mpm_RegisterMemPool */
	unregister_mempool,	/* mpm_UnregisterMemPool */
	getmempooldesc_byid,	/* mpm_GetMemPoolDesc_ByID */
	getmempooldesc_byname,	/* mpm_GetMemPoolDesc_ByName */
	memalloc,		/* mpm_MemAlloc */
	memfree,		/* mpm_MemFree */
	NULL,			/* mpm_MemAvail */

	obtainlayoutid,		/* mpm_ObtainLayoutID */
	releaselayoutid,	/* mpm_ReleaseLayoutID */
	getlayoutdesc,		/* mpm_GetLayoutDesc_ByID */
	
	align_up,		/* mpm_AlignUp */
	align_down		/* mpm_AlignDown */
};

_EXPORT mempool_module	*modules[] = {
	&mpmod,
	NULL
};
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                /* :ts=8 bk=0
 *
 * testmem.c:	Test to make sure my memory allocator is sane.
 *
 * gcc -I$BUILDHOME/src/kit/dinky/include -I$BUILDHOME/src/kit/surface/include testmem.c mem.c -ldinky
 *
 * Leo L. Schwab					1999.01.21
 */
#include <kernel/OS.h>
#include <surface/mempool.h>
#include <stdio.h>
#include <stdlib.h>

#include "mem.h"


#define	NNODES	4096
#define	NTESTS	2048

#define	TESTAREASIZE	(16<<20)

int32 checkfreelist (int32 *nnodes);
void freeseqandcheck (void **ptrs, int32 *sizes, int32 n);
void freerndandcheck (void **ptrs, int32 *sizes, int32 n);

void *testalloc (int32 size, uint32 carebits, uint32 statebits);
void testfree (void *ptr, int32 size);

status_t initmem (uint32 size);


static MemList	gml;
static MemNode	gnodes[NNODES],
		*gnodeptrs[NNODES];
static int32	gfirstfree;

static void	*gptrs[N