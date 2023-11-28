isnode->fn_Next)
	{
		if ((uint32) rn < (uint32) thisnode)
			break;
	}

	/*
	 * I walked through all six cases by hand.  This should work...
	 */
	newnode = (FreeNode *) rn;
	newnode->fn_Next = thisnode;
	newnode->fn_Size = size;

	if (prevnode) {
		if ((uint32) prevnode + prevnode->fn_Size == (uint32) newnode)
		{
			prevnode->fn_Size += newnode->fn_Size;
			newnode = prevnode;
		} else
			prevnode->fn_Next = newnode;
	} else
		mp->mp_firstfreenode = newnode;

	if (thisnode) {
		if ((uint32) newnode + newnode->fn_Size == (uint32) thisnode)
		{
			newnode->fn_Next = thisnode->fn_Next;
			newnode->fn_Size += thisnode->fn_Size;
		}
	}

#if 0
/*
 * Well, I did this totally wrong (last entry in list is not necessarily
 * at the highest address).  I'll worry about this later...
 */
	/*  See if it's worth trying to size the area down.  */
	if (prevnode  &&  !thisnode) {
		size_t	thresh = 4 * B_PAGE_SIZE;

		if (prevnode->fn_Size > thresh) {
			thresh += mp->mp_rangel