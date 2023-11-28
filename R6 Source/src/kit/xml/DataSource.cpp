);
	     rn = (BRangeNode *) NEXTNODE_OP (base, rn))
	{
		if (rn->rn_MR.mr_Offset == offset) {
			RemNode_OP (base, (Node_OP *) rn);
			mp->m