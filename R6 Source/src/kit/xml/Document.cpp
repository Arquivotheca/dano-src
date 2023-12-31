/* :ts=8 bk=0
 *
 * ranges.h:	MemRange prototypes.
 *
 * Leo L. Schwab					1999.08.10
 */
#ifndef	_RANGES_H
#define	_RANGES_H

#ifndef	_METAPOOL_H
#include "metapool.h"
#endif

#ifdef __cplusplus
extern "C" {
#endif


extern status_t procurerange (struct BMemRange *mr, struct mempool *mp);
extern status_t returnrange (struct BMemRange *mr, struct mempool *mp);
extern status_t logrange (struct BRangeNode *rn, struct mempool *mp);
extern struct BRangeNode *unlogrange (uint32 offset, struct mempool *mp);


#ifdef __cplusplus
}
#endif

#endif	/*  _RANGES_H  */
                                                                                                                                                                                                                                                                                                                                                                                                                                                                             /* :ts=8 bk=0
 *
 * testgp.c:	Test to make sure my memory allocator is sane.
 *
 * gcc -Iinclude -I$BUILDHOME/src/kit/dinky/include -I$BUILDHOME/src/kit/surface/include testgp.c genpool.c ranges.c metapool.c defalloc.c -ldinky
 *
 * Leo L. Schwab					1999.08.12
 */
#include <kernel/OS.h>
#include <drivers/genpool_module.h>
#include <stdio.h>
#include <stdlib.h>

#include "metapool.h"


#ifndef	B_MEMSPECF_FROMTOP
#define	B_MEMSPECF_FROMTOP	1
#endif

#define	NNODES	4096
#define	NTESTS	2048

#define	TESTAREASIZE	(16<<20)


int32 checkfreelist (struct BPoolInfo *pi, int32 *nnodes);
int32 checkalloclist (struct BPoolInfo *pi, int32 *nnodes);
void freeseqandcheck (void **ptrs, int32 *sizes, int32 n);
void freerndandcheck (void **ptrs, int32 *sizes, int32 n);

void *testalloc (int32 size, uint32 carebits, uint32 statebits);
void testfree (void *ptr, int32 size);
status_t allocalignedrange (struct BMemRange *mr,
			    struct BPoolInfo *pi,
			    uint32 size,
			    uint32 carebits,
			    uint32 statebits,
			    uint32 allocflags);

uint32 align_up (uint32 base_orig, uint32 carebits, uint32 statebits);
uint32 align_down (uint32 base_orig, uint32 carebits, uint32 statebits);
static int highbit (uint32 val);

status_t initmem (uint32 size);
void shutdownmem (void);



extern genpool_module	*modules[];

static genpool_module	*gpm;
static BPoolInfo	*gpoolptr;
static area_id		pool_aid = -1;
static void		*gptrs[NTESTS];
static int32		gsizes[NTESTS];

static int		gDEBUG;


int
main (int ac, char **av)
{
	register int	i;
	int32		size, total;
	uint32		care, state;
	status_t	retval;
	
	if ((retval = initmem (TESTAREASIZE)) < 0)
		printf ("Can't initialize memory manager: %d (0x%08lx)\n",
			retval, retval);
	printf ("Allocated %d for workspace.\n", TESTAREASIZE);

	printf ("\nNormal allocations, 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
	printf ("Nodes on free list:  %d\n", size);
	
	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 4K-64K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 65536 - 4096))
			;
		size += 4096;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nNormal allocations, 4K-64K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 65536 - 4096))
			;
		size += 4096;
		if (gptrs[i] = testalloc (size, 0, 0)) {
			gsizes[i] = size;
			total += size;
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
	printf ("Nodes on free list:  %d\n", size);
	
	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, simple mask (0-6 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		if (care = random () % 7) {
			care = (1 << care) - 1;
			state = random() & care;
		}
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, simple mask (0-6 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		if (care = random () % 7) {
			care = (1 << care) - 1;
			state = random() & care;
		}
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("RANDOM free and check:\n");
	freerndandcheck (gptrs, gsizes, NTESTS);


	printf ("\nAligned allocations, complex mask (8 bits), 0-4K.\n");
	for (i = total = 0;  i < NTESTS;  i++) {
		while (!(size = random () % 4096))
			;
		size += 4096;
		care = random () % 0xFF;
		state = random () & care;
		if (gptrs[i] = testalloc (size, care, state)) {
			gsizes[i] = size;
			total += size;
			if (care  &&  ((uint32) gptrs[i] & care) != state)
				printf ("Misaligned allocation: \
expected 0x%02x, got 0x%02x\n", state, (int32) gptrs[i] & care);
		} else {
			printf ("Allocation #%d failed, size %d\n", i, size);
			gsizes[i] = -1;
		}
	}
	printf ("Total bytes allocated: %d\n", total);
	printf ("Reported bytes allocated: %d\n", checkalloclist (gpoolptr, &size));
	printf ("Nodes on alloc list: %d\n", size);
	printf ("Expected free space: %d\n", TESTAREASIZE - total);
	printf ("Reported free space: %d\n", checkfreelist (gpoolptr, &size));
	printf ("Nodes on free list:  %d\n", size);

	printf ("SEQUENTIAL free and check:\n");
	freeseqandcheck (gptrs, gsizes, NTESTS);


	shutdownmem ();
	return (0);
}


int32
checkfreelist (struct BPoolInfo *pi, int32 *nnodes)
{
	BRangeNode	*rn;
	int32		total = 0;
	void		*base;

	if (nnodes)	*nnodes = 0;
	base = pi->pi_RangeLists;
	for (rn = (BRangeNode *) FIRSTNODE_OP (base, &((BRangeList *) base)->rl_List);
	     NEXTNODE_OP (base, rn);
	     rn = (BRangeNode *) NEXTNODE_OP (base, rn))
	{
#if 0
if (gDEBUG) {
 printf ("RangeNode #%d @ 0x%08lx:\n", *nnodes, rn);
 printf ("      rn_n_Next: 0x%08lx\n", rn->rn_Node.n_Next);
 printf ("      rn_n_Prev: 0x%08lx\n", rn->rn_Node.n_Prev);
 printf ("    rn_UserSize: %d\n", rn->rn_UserSize);
 printf ("   rn_mr_Of