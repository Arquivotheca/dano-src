*******************************
 * These routines manipulate the alloclist, the list that records allocated
 * ranges.
 */
/*
 * Record an obtained range in the alloclist.
 */
status_t
logrange (struct BRangeNode *rn, struct mempool *mp)
{
	BRangeNode	*where;
	void		*base;

	/*
	 * Insert in order of mr_Offset.
	 */
	base = mp->mp_pi.pi_RangeLists;
	for (where = (BRangeNode *)
	      FIRSTNODE_OP (base, &mp->mp_alloclist->rl_List);
	     NEXTNODE_OP (base, where);
	     where = (BRangeNode *) NEXTNODE_OP (base, where))
	{
		if (where->rn_MR.mr_Offset > rn->rn_MR.mr_Offset)
			break;
	}
	InsertNodeBefore_OP (base, (Node_OP *) rn, (Node_OP *) where);
	mp->mp_alloclist->rl_Total += rn->rn_MR.mr_Size;
	return (B_OK);
}

/*
 * Delist an obtained range from the alloclist.
 */
stru