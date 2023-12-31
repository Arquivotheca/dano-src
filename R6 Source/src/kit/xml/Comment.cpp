B_PAGE_SIZE - 1);
printf ("Sizing RangeLists down from %d to %d...", mp->mp_rangelistsize, thresh);
			
			if (resize_area
			     (mp->mp_pi.pi_RangeLists_AID, thresh) == B_OK)
			{
printf ("Success!");
				prevnode->fn_Size -= mp->mp_rangelistsize -
						     thresh;
				mp->mp_rangelistsize = thresh;
			}
printf ("\n");
		}
	}
#endif
}


void
initrangenodepool (struct mempool *mp, uint32 size, uint32 initialreserved)
{
	FreeNode *basenode;

	initialreserved = (initialreserved + 7) & ~7;
	basenode = (FreeNode *)
		   ((uint32) mp->mp_pi.pi_RangeLists + initialreserved);

	basenode->fn_Next = NULL;
	basenode->fn_Size = size - initialreserved;
	mp->mp_firstfreenode = basenode;
	
	/*
	 * Create a "ripcord"; an emergency backup RangeNode for returnrange()
	 * in case it needs a free RangeNode and can't get one normally.
	 */
	mp->mp_ripcord = allocrangenode (mp, 0);
	mp->mp_ripcordpulled = FALSE;
	memset (mp->mp_ripcord, 0, sizeof (BRangeNode));
}
                                                               /* :ts=8 bk=0
 *
 * metapool.h:	Definitions for internal memory management.
 *
 * Leo L. Schwab					1999.01.04
 */
#ifndef _METAPOOL_H
#define	_METAPOOL_H

#ifndef _OS_H
#include <kernel/OS.h>
#endif
#ifndef _GENPOOL_H
#include <surface/genpool.h>
#endif

#ifdef __cplusplus
extern "C" {
#endif


/*
 * Each region of free space is represented by a FreeNode.
 */
typedef struct FreeNode {
	struct FreeNode	*fn_Next;
	uint32		fn_Size;