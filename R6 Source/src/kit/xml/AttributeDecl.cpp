
	default:
		return (B_ERROR);
	}
}


static genpool_module	gpmod = {
	{
		B_GENPOOL_MODULE_NAME,
		B_KEEP_LOADED,
		std_ops
	},
	allocpoolinfo,			/*  gpm_AllocPoolInfo		*/
	freepoolinfo,			/*  gpm_FreePoolInfo		*/
	createpool,			/*  gpm_CreatePool		*/
	clonepool,			/*  gpm_ClonePool		*/
	deletepool_id,			/*  gpm_DeletePool		*/
	findpool,			/*  gpm_FindPool		*/
	NULL,				/*  gpm_GetNextPoolID		*/
	getuniqueownerid,		/*  gpm_GetUniqueOwnerID	*/

	lockpool,			/*  gpm_LockPool		*/
	unlockpool,			/*  gpm_UnlockPool		*/

	markrange_id,			/*  gpm_MarkRange		*/
	unmarkrange_id,			/*  gpm_UnmarkRange		*/
	unmarkrangesownedby,		/*  gpm_UnmarkRangesOwnedBy	*/

	allocbymemspec,			/*  gpm_AllocByMemSpec		*/
	freebymemspec			/*  gpm_FreeByMemSpec		*/
};

/*
 * Old version 1 module structure, which doesn't have the gpm_GetUniqueOwnerID
 * vector.  We need this to remain binary-compatible.  This can be deleted
 * once we've shipped this new version.
 */
#ifdef V1_COMPAT

typedef struct genpool_module_v1 {
	module_info	gpm_ModuleInfo;
	struct BPoolInfo *
			(*gpm_AllocPoolInfo) (void);
	void		(*gpm_FreePoolInfo) (struct BPoolInfo *pi);
	status_t	(*gpm_CreatePool) (struct BPoolInfo *pi,
					   uint32 maxallocs,
					   uint32 userdatasize);
	status_t	(*gpm_ClonePool) (struct BPoolInfo *pi);
	status_t	(*gpm_DeletePool) (uint32 poolid);
	uint32		(*gpm_FindPool) (const char *name);
/**/	uint32		(*gpm_GetNextPoolID) (uint32 *cookie);

	status_t	(*gpm_LockPool) (uint32 poolid);
	void		(*gpm_UnlockPool) (uint32 poolid);

	status_t	(*gpm_MarkRange) (uint32 poolid,
					  struct BMemRange *cl_mr,
					  const void *userdata,
					  uint32 userdatasize);
	status_t	(*gpm_UnmarkRange) (uint32 poolid, uint32 offset);
	status_t	(*gpm_UnmarkRangesOwnedBy) (uint32 poolid,
						    uint32 owner);

	/*  Default allocation logic  */
	status_t	(*gpm_AllocByMemSpec) (struct BMemSpec *ms);
	status_t	(*gpm_FreeByMemSpec) (struct BMemSpec *ms);
} genpool_module_v1;


static genpool_module_v1	gpmod_v1 = {
	{
		"generic/genpool/v1",
		B_KEEP_LOADED,
		std_ops
	},
	allocpoolinfo,			/*  gpm_AllocPoolInfo		*/
	freepoolinfo,			/*  gpm_FreePoolInfo		*/
	createpool,			/*  gpm_CreatePool		*/
	clonepool,			/*  gpm_ClonePool		*/
	deletepool_id,			/*  gpm_DeletePool		*/
	findpool,			/*  gpm_FindPool		*/
	NULL,				/*  gpm_GetNextPoolID		*/

	lockpool,			/*  gpm_LockPool		*/
	unlockpool,			/*  gpm_UnlockPool		*/

	markrange_id,			/*  gpm_MarkRange		*/
	unmarkrange_id,			/*  gpm_UnmarkRange		*/
	unmarkrangesownedby,		/*  gpm_UnmarkRangesOwnedBy	*/

	allocbymemspec,			/*  gpm_AllocByMemSpec		*/
	freebymemspec			/*  gpm_FreeByMemSpec		*/
};

#endif	/*  V1_COMPAT  */


_EXPORT module_info	*modules[] = {
	(module_info *) &gpmod,
#ifdef V1_COMPAT
	(module_info *) &gpmod_v1,
#endif
	NULL
};
                                                                                                                                                                                                                                                                                                 /* :ts=8 bk=0
 *
 * metapool.c:	Routines to manage the free space in the RangeLists
 *		themselves.
 *
 * Leo L. Schwab					1999.08.10
 */
#include <surface/genpool.h>
#include <stdio.h>

#include "metapool.h"


/*
 * Since only we have write privileges to the area containing the RangeLists,
 * we can pull this old, old trick: The freelists are maintained in the free
 * memory areas themselves.  This means that the nodes themselves move, which
 * makes the game slightly more interesting.
 */
struct BRangeNode *
allocrangenode (struct mempool *mp, uint32 usersize)
{
	FreeNode	*prevnode, *thisnode;
	int		retrying = FALSE;

	if (mp->mp_ripcordpulled)
		return (NULL);

	/*  Add size of RangeNode, and round up to nearest multiple of 8.  */
	usersize = (usersize + sizeof (BRangeNode) + 7) & ~7;

	/*
	 * Find a free area of sufficient size.
	 */
retry:	for (prevnode = NULL, thisnode = mp->mp_firstfreenode;
	     thisnode;
	     prevnode = thisnode, thisnode = thisnode->fn_Next)
	{
		if (thisnode->fn_Size >= usersize)
			break;
	}

	if (thisnode) {
		if (thisnode->fn_Size == usersize) {
			/*  Exactly consumed block, bye-bye  */
			if (prevnode)
				prevnode->fn_Next = thisnode->fn_Next;
			else
				mp->mp_firstfreenode = thisnode->fn_Next;
		} else {
			/*  Relocate node, shrinking by size of request  */
			FreeNode	*movednode;

			movednode = (FreeNode *) ((uint32) thisnode +
						  usersize);
			movednode->fn_Next = thisnode->fn_Next;
			movednode->fn_Size = thisnode->fn_Size - usersize;
			if (movednode->fn_Size < sizeof (FreeNode)) {
				kprintf (">>> Deep bandini in node allocator; \
fn_Size = %d\n", movednode->fn_Size);
				return (NULL);
			}
			if (prevnode)
				prevnode->fn_Next = movednode;
			else
				mp->mp_firstfreenode = movednode;
		}
		return ((BRangeNode *) thisnode);
	} else {
		status_t	retval;
		uint32		newsize;

		/*  Resize this area to make room and try again.  */
//printf ("Sizing RangeLists up.\n");
		if (retrying)
			/*  Oops, we already tried that...  */
			return (NULL);
		
		newsize = mp->mp_rangelistsize + B_PAGE_SIZE;
//printf ("...from %d to %d...\n", mp->mp_rangelistsize, newsize);
		if ((retval = resize_area (mp->mp_pi.pi_RangeLists_AID, newsize)) < 0)
		{
//printf ("resize_area() failed; retval = %d (0x%08lx)\n", retval, retval);
			return (NULL);
		}

		if (prevnode)
			prevnode->fn_Size += B_PAGE_SIZE;
		else {
			/*  Woah, we're full to the rafters!  */
			mp->mp_firstfreenode =
			 (FreeNode *) ((uint32) mp->mp_pi.pi_RangeLists +
				       mp->mp_rangelistsize);
			mp->mp_firstfreenode->fn_Next = NULL;
			m