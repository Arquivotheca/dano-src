de	*rn;
	status_t	retval;

	if (rn = unlogrange (offset, mp)) {
		uint32	idx;

		idx = rn->rn_MR.mr_LockIdx;
		if (atomic_add (mp->mp_pi.pi_RangeLocks + idx, 1) > 0) {
			/*  In use; put it back.  */
			/*  FIXME: This could be more efficient.  */
			atomic_add (mp->mp_pi.pi_RangeLocks + idx, -1);
			logrange (rn, mp);
			return (B_POOLERR_MEMRANGE_LOCKED);
		}

		/*
		 * returnrange() may cause mp_ripcord to get pulled, so
		 * be sure to freerangenode(rn) as soon as possible so it
		 * can get recovered.
		 */
		retval = returnrange (&rn->rn_MR, mp);
//if (retval < 0)
// printf ("=== returnrange() returned %d (0x%08lx)\n", retval, retval);
		/*  Free rangelock  */
		disposerangelock (mp, idx);

		/*  Delete RangeNode  */
		freerangenode (mp, rn);
		return (retval);
	} else {
//printf ("=== Bad Offset supplied to unlogrange(): 0x%08lx\n", offset);
		return (B_POOLERR_UNKNOWN_OFFSET);
	}
}


static status_t
unmarkrange_id (uint32 poolid, uint32 offset)
{
	mempool	*mp;

	if (!(mp = lookuppool (poolid)))
		return (B_POOLERR_UNKNOWN_POOLID);

	return (unmarkrange (mp, offset));
}

static status_t
unmarkrangesownedby (uint32 poolid, uint32 owner)
{
	BRangeNode	*rn, *next;
	mempool		*mp;
	status_t	retval = B_OK;
	void		*base;

	if (!(mp = lookuppool (poolid)))
		return (B_POOLERR_UNKNOWN_POOLID);

	base = mp->mp_pi.pi_RangeLists;
	for (rn = (BRangeNode *)
	      FIRSTNODE_OP (base, &mp->mp_alloclist->rl_List);
	     next = (BRangeNode *) NEXTNODE_OP (base, rn);
	     rn = next)
	{
		if (rn->rn_MR.mr_Owner == owner) {
			uint32	idx;

			idx = rn->rn_MR.mr_LockIdx;
			if (atomic_add (mp->mp_pi.pi_RangeLocks + idx, 1) > 0)
			{
				/*  In use; put it back.  */
				atomic_add (mp->mp_pi.pi_RangeLocks + idx, -1);
				retval = B_POOLERR_MEMRANGE_LOCKED;
				continue;
			}
			RemNode_OP (base, (Node_OP *) rn);
			mp->mp_alloclist->rl_Total -= rn->rn_MR.mr_Size;

			returnrange (&rn->rn_MR, mp);

			/*  Free rangelock  */
			disposerangelock (mp, idx);

			/*  Delete RangeNode  */
			freerangenode (mp, rn);
		}
	}
	return (retval);
}



/*****************************************************************************
 * Module setup/teardown.
 */
static status_t
init (void)
{
	status_t retval;

	if ((retval = BInitOwnedBena4 (&gLock,
				  