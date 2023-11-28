/*
 * SCCSID = "@(#)shmem.h	13.4 03/20/94"
 */
#ifndef SHMEM_HEADER
#define SHMEM_HEADER

/*
 * Definitions for shmem.c for managing shared memory allocator.
 *
 * Memory allocations for tables are made from a pool of shared memory
 * segments.  Each segment is divided into free and used blocks.  Each
 * process has its own linked list of structures (struct shmem_seg)
 * describing the memory segments attached to the process.  The segments
 * may be mapped into more than one process and so cannot contain any
 * absolute memory address references.
 *
 * Each segment begins with a short header (struct shmem_seg_hdr) which
 * contains the shared memory id, the size of the segment, the size of the
 * largest free block, an integer offset to the first free block, and a
 * lock.  Since processes are running asynchronously, segments must be
 * locked by a process before modifying any of the descriptor data.  The
 * segment header is followed by a sequence of free and used blocks.
 *
 * Each block within a segment begins with a short descriptor (struct
 * shmem_seg_blk) which contains the offset to the beginning of the
 * segment, the size of the block, the size of the next and previous
 * blocks (for traversing the list of blocks forward and backwards), a
 * reference count indicating how many processes are using the block, and
 * if the block is free (reference count is zero), the offset to the next
 * free block.
 *
 *	first_seg
 *	   |
 *	   |	+----------+	+----------+	+----------+
 *	   +-->	| seg 1    |-->	| seg 2    |-->	| seg 3    |--> NULL
 *		|          |   	|          |   	|          |
 *		+----------+	+----------+	+----------+
 *  descriptors	     |		     |		     |		Local memory
 *  -------------------------------------------------------------------------
 *  segments	     |		     |		     |		Shared memory
 *		     V		     V		     V
 *		+----------+	+----------+	+----------+
 *		| hdr      |	| hdr      |	| hdr      |
 *		|----------|	|----------|	|----------|
 *		| blk1     |	| blk1     |	| blk1     |
 *		|          |	|----------|	|          |
 *		|          |	| blk2     |	|          |
 *		|          |	|          |	|          |
 *		|----------|	|----------|	|          |
 *		| blk2     |	| . . .    |	|          |
 *		|----------|	+----------+	|          |
 *		| . . .    |			|          |
 *		+----------+			|          |
 *						+----------+
 *
 *
 * The first segment descriptor is pointed to by the static global "shmem_first_seg"
 * (which is NULL if no segments are attatched to the process).
 */
struct shmem_seg_hdr {
	int	sh_first_free;	/* offset to first free block (0 if none free).
				   NOTE: must be first member of structure!! */

	int	sh_size;	/* size of this segment */
	int	sh_perm;	/* permissions of this segment */
	int	sh_memid;	/* memid of this segment */
	int	sh_maxfree;	/* size of largest free block */
	int	sh_lock;	/* segment lock for asynchronous access */
};

struct shmem_seg_blk {
	int	sb_next_free;	/* offset to next free block:
				 *  > 0 for free block,
				 *  = 0 for last free block,
				 *  < 0 (-1) for used block,
				 * NOTE: must be first member of structure!! */

	int	sb_seg_offset;	/* offset from beginning of segment */

	int	sb_size;	/* size of this block (always double even) */
	int	sb_size_nxt;	/* size of next block (0 for last block) */
	int	sb_size_pre;	/* size of previous block (0 for 1st block) */
				/* NOTE; 'size' members only change when we
				 * merge or split blocks */

	int	sb_refcnt;	/* global reference count for this block.
				   ( = 0 for free block ).  This is used to
				   determine if the block can be returned to
				   the pool of free blocks within a segment. */
};

struct shmem_seg {
	struct shmem_seg	* seg_next;	/* ^ to next seg descriptor */
	struct shmem_seg_hdr	* seg_hdr;	/* ^ to segment start */
	int			seg_refcnt;	/* local segment reference count.
						   Is equal to the number of blocks
						   in segment used by this process.
						   When goes to zero, segment
						   can be detached from process. */
};


#define BLK_ADD_OFF(blk,off)	((struct shmem_seg_blk *)((char *)(blk) + (off)))
#define BLK_SUB_OFF(blk,off)	((struct shmem_seg_blk *)((char *)(blk) - (off)))
#define BLK_DIFF(blk1,blk2)	((char *)(blk1) - (char *)(blk2))
#define BLK_IS_FREE(blk)	((blk)->sb_next_free >= 0)
#define BLK_IS_LAST(blk)	((blk)->sb_size_nxt == 0)
#define BLK_IS_FIRST(blk)	((blk)->sb_size_pre == 0)
#define BLK_IS_LAST_FREE(blk)	((blk)->sb_next_free == 0)
#define BLK_NEXT(blk)		BLK_ADD_OFF ((blk), (blk)->sb_size)
#define BLK_PREV(blk)		BLK_SUB_OFF ((blk), (blk)->sb_size_pre)

/* get pointer to segment start from address of block
 */
#define BLK_SEG_HDR(blk)	((struct shmem_seg_hdr *)((char *)(blk) - (blk)->sb_seg_offset))

/* get pointer to block data from address of block
 */
#define BLK_ADDR(blk)		((char *)(blk) + sizeof(struct shmem_seg_blk))

/* get pointer to block from address of data
 */
#define ADDR_BLK(addr)		BLK_SUB_OFF((addr), sizeof(struct shmem_seg_blk))

/* loop over all segments starting at seg_start.
 * 'seg' will be NULL at normal termination.
 */
#define SEG_LOOP(seg,seg_start)	for (seg = seg_start;\
				     seg != NULL;\
				     seg = seg->seg_next )

/* loop over all free blocks but the last in segment (header).
 * 'blk' will point to the last free block at normal termination
 */
#define BLK_LOOP(blk,hdr)	for ( blk = BLK_ADD_OFF (hdr, hdr->sh_first_free);\
				      blk->sb_next_free > 0;\
				      blk = BLK_ADD_OFF (blk, blk->sb_next_free) )

#define SEG_MIN_SIZE		(64*1024)
#define BLK_MIN_SIZE		(sizeof(struct shmem_seg_blk) + 1028)

/*
 * SEG_LOCK locks a particular segment against access by an
 * asynchronous event in another process.  It uses a special test and
 * set mechanism provided soley for this purpose.  If the segment is
 * currently locked, it loops indefinitely until it becomes unlocked.
 * The result is always a locked segment.
 *
 * SEG_UNLOCK unlocks a segment locked by SEG_LOCK.
 */
#define SEG_LOCK(hdr)	while (!test_and_set (&hdr->sh_lock))
#define SEG_UNLOCK(hdr)	(hdr->sh_lock = 0)

/*
 * LOCAL_LOCK locks all access to the shared memory structures from
 * asynchronous events occuring in the same process.  This is necessary
 * since such an event could try to obtain a lock on a segment and loop
 * forever waiting.  Also, modifications to the linked list of segment
 * descriptors (or traversal through it) could be disrupted by an
 * asynchronous event in the same process.
 *
 * LOCAL_UNLOCK unlocks the structures locked by LOCAL_LOCK.
 */
#define LOCAL_LOCK()	test_and_set (&shmem_local_lock)
#define LOCAL_UNLOCK()	(shmem_local_lock = 0)

extern char	* shmalloc ();
extern void	shmfree ();
extern char	* shmmap ();
extern int	shmblkid ();
extern int	shmfree_id ();

#endif

ÿ