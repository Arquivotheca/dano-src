node (struct mempool *mp, struct BRangeNode *rn);
extern void initrangenodepool (struct mempool *mp,
			       uint32 size,
			       uint32 initialreserved);


#ifdef __cplusplus
}
#endif

#endif	/*  _METAPOOL_H  */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                                       /* :ts=8 bk=0
 *
 * ranges.c:	Routines to allocate and free MemRanges.
 *
 * Leo L. Schwab					1999.08.10
 */
#include <surface/genpool.h>
#include <stdio.h>

#include "ranges.h"


/*****************************************************************************
 * These routines manipulate the freelist, the list that enumerates all
 * unallocated ranges.
 */
/*
 * Procures a range of bytes from the freelist.
 */
status_t
procurerange (struct BMemRange *mr, struct mempool *mp)
{
	BRangeNode	*freenode, *newnode;
	status_t	retval;
	uint32		freebase, allocbase, allocend;
	uint32		size;
	void 		*base;

	if (!(size = mr->mr_Size))
		return (B_BAD_VALUE);

	base = mp->mp_pi.pi_RangeLists;
	freenode = NULL;
	allocend = (allocbase = mr->mr_Offset) + mr->mr_Size;

	/*
	 * Search for a free range surrounding the client's request.
	 */
	for (newnode = (BRangeNode *)
	      FIRSTNODE_OP (base, &mp->mp_freelist->rl_List);
	     NEXTNODE_OP (base, newnode);
	     newnode = (BRangeNode *) NEXTNODE_OP (base, newnode))
	{
		if (newnode->rn_MR.mr_Offset >= allocend  ||
		    newnode->rn_MR.mr_Offset + newnode->rn_MR.mr_Size <=
		     allocbase)
			/*  Definitely not intersecting the client's range  */
			continue;

		if (newnode->rn_MR.mr_Offset > allocbase  ||
		    newnode->rn_MR.mr_Offset + newnode->rn_MR.mr_Size <
		     allocend)
			/*  Client range overlaps already-reserved range  */
			return (B_POOLERR_MEMRANGE_UNAVAILABLE);

		/*  Found an encompassing range.  */
		freenode = newnode;
		freebase = freenode->rn_MR.mr_Offset;
		break;
	}

	if (freenode) {
		if (allocbase == freebase) {
			if (size == freenode->rn_MR.mr_Size) {
				/*  Exactly consumed block, bye-bye  */
				RemNode_OP (base, (Node_OP *) freenode);
				freerangenode (mp, freenode);
			} else {
				/*  Shrink free block by allocation  */
				freenode->rn_MR.mr_Offset += size;
				freenode->rn_MR.mr_Size -= size;
			}
		} else {
			/*
			 * allocbase is greater than freebase; we may
			 * need to create a new RangeNode.
			 */
			if (allocbase + size ==
			    freebase + freenode->rn_MR.mr_Size)
			{
				/* Exact fit into end; shorten block */
				freenode->rn_MR.mr_Size -= size;
			} else {
				/*
				 * The icky case.  Create a new
				 * free block; describe free areas
				 * before and after allocated region.
				 */
				/* Create new block after allocation */
				if (!(newnode = allocrangenode (mp, 0)))
					goto nomem;	/*  Look down  */
				newnode->rn_MR.mr_Offset =
				 allocbase + size;
				newnode->rn_MR.mr_Size =
				 freebase + freenode->rn_MR.mr_Size -
				 allocbase - size;
				InsertNodeAfter_OP (base,
						    (Node_OP *) newnode,
						    (Node_OP *) freenode);

				/*  Shorten existing block  */
				freenode->rn_MR.mr_Size = allocbase - freebase;
			}
		}
		mp->mp_freelist->rl_Total -= size;
		retval = B_OK;
	} else
nomem:		retval = B_NO_MEMORY;

	return (retval);
}

/*
 * Return a range to the free list.
 */
status_t
returnrange (struct BMemRange *mr, struct mempool *mp)
{
	register BRangeNode	*curr, *prev;
	uint32			size, offset;
	void			*base;

	/*
	 * Search for the correct nodes in which to place the newly freed
	 * region.
	 ****
	 * FIXME: There should be a debugging version that checks if the area
	 * to be freed overlaps existing free regions, indicating client
	 * screwup.
	 */
	offset = mr->mr_Offset;		/*  zero is a valid offset  */
	if (!(size = mr->mr_Size))
		return (B_BAD_VALUE);

//printf ("--- Freeing %d @ 0x%08lx\n", size, offset);
	base = mp->mp_pi.pi_RangeLists;
	for (prev = NULL, curr = (BRangeNode *)
			   FIRSTNODE_OP (base, &mp->mp_freelist->rl_List);
	     NEXTNODE_OP (base, curr);
	     prev = curr, curr = (BRangeNode *) NEXTNODE_OP (base, curr))
	{
		if (curr->rn_MR.mr_Offset > offset)
			break;
	}
	if (NEXTNODE_OP (base, curr)  &&
	    offset + size == curr->rn_MR.mr_Offset)
	{
//printf ("--- Prepend\n");
		/*  Exactly prepends to this block.  */
		curr->rn_MR.mr_Offset = offset;
		curr->rn_MR.mr_Size += size;
		if (prev  &&
		    prev->rn_MR.mr_Offset + prev->rn_MR.mr_Size == offset)
		{
			/*  Coalesce current and previous blocks.  */
//printf ("---- Coalesce\n");
			prev->rn_MR.mr_Size += curr->rn_MR.mr_Size;
		