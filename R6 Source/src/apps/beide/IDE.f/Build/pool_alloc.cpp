/*/  Metrowerks Standard Library  Version 1.2  07-May-96  /*/

/*
 *	pool_alloc.c
 *	
 *		Copyright © 1995-1996 Metrowerks, Inc.
 *		All rights reserved.
 *	
 *	Routines
 *	--------
 *		__init_pool_obj
 *		__pool_preallocate
 *		__pool_preassign
 *		__pool_alloc
 *		__pool_alloc_clear
 *		__pool_realloc
 *		__pool_free
 *	
 *	Description
 *	-----------
 *	
 *		Although a host operating system will usually provide memory management
 *		primitives, they are often not very efficient at the management of many
 *		small to medium sized blocks of nonrelocatable storage. These routines
 *		provide for an efficient system of memory pool management on top of such
 *		host primitives.
 *	
 *	Implementation
 *	--------------
 *	
 *		The dynamic allocation scheme used here is based in large part on
 *		techniques presented in Section 2.5 of Knuth's "The Art of Computer
 *		Programming, Vol. 1".
 *		
 *		Basically, allocated memory is drawn from a circular, doubly-linked list of
 *		available blocks of memory. The list is kept in no particular order and the
 *		first block encountered in the list that is large enough to satisfy an
 *		allocation request is used.
 *		
 *		Excess bytes in a block are split off and returned to the list. However,
 *		pad bytes needed to maintain a given storage alignment are not split off.
 *		Furthermore, if the number of excess bytes remaining is less than a certain
 *		amount, then no bytes are split off from the allocated block to prevent
 *		excessive fragmentation.
 *		
 *		Each available block of memory begins and ends with a special "tag" field.
 *		This tag identifies the size of the block. In addition, the tag at the
 *		beginning of a block indicates whether the block is available or not and
 *		also whether the immediately preceding block is available or not. This
 *		information is used when a block is deallocated to determine whether or not
 *		to merge adjacent blocks.
 *		
 *		Traditionally, information about the availability of the preceding block
 *		would be kept in a reserved area at the end of the block, and this would
 *		necessarily be so even when the block is in use. This, however, would waste
 *		storage since only a single bit is required but one or more bytes woiuld
 *		need to be reserved for it (usually at least four to maintain a desired
 *		storage alignment).
 *		
 *		When an allocation request is made that cannot be satisfied by any
 *		available block, an attempt is made to acquire a block of memory from the
 *		host. In order to distinguish such a block from the kind of blocks we are
 *		managing, we shall call it a heap. Enough bytes are requested from the host
 *		to satisfy the request, including any bytes needed for overhead, but never
 *		less than some minimum amount. The heap is made to look like an available
 *		block and then passed on to satisfy the request. If fewer bytes are needed
 *		than we acquired, then the block is split with excess bytes made available
 *		as above.
 *		
 *		Initially, there are no available blocks preallocated, and therefore the
 *		first allocation request will necessarily require the acquisition of a
 *		heap.
 *		
 *		Since the tag field will have to be reserved for every block, whether
 *		allocated or not, we will want to ensure that it consumes as little storage
 *		as possible. We need only a few bits for availability info. Block size
 *		needs to be larger than a short int (16 bits), but not necessarily as large
 *		as a full long int (32 bits). Therefore, we ought to be able to pack
 *		everything into a single 4-byte field.
 *		
 *		The "proper" way to do this in C would be to use bitfields. However, the
 *		specification of bitfields is so riddled with implementation-defined areas
 *		that we cannot ensure that their use would be effective on all platforms.
 *		Therefore, we will instead use the & and | operators to manipulate the bit
 *		quantities. We will avoid having to use << and/or >> on the size quantity
 *		by asserting that the size of all blocks must be a multiple of 4 bytes
 *		(which is a useful assertion for performance reasons as well), giving us
 *		use of the 2 low-order bits for availability.
 *		
 *		In addition, we will make the (not unreasonable) assertion that the size of
 *		a block must always be non-negative. This gives us use of the high-order
 *		sign bit as well. However, trying to isolate the sign bit is difficult to
 *		do in an implementation-independent manner, so we will simply assert that
 *		any negative value has special significance.
 *		
 *		In particular, we will ensure that each host-allocated block begins and
 *		ends with a tag indicating a size of -1L so that we can identify the
 *		boundaries of the block from the inside at deallocation time. This serves
 *		two purposes: (1) it ensures that memory preceding or following the heap
 *		never inadvertantly becomes a candidate for merging and (2) it permits
 *		detection of when the entire heap becomes available again, at which point
 *		it may be returned to the host if desired.
 *		
 *		Here is a graphical spin on things that hopefully makes things a little
 *		clearer:
 *	
 *					--------------------------
 *					| 0 |    size    | p | t |			block start tag
 *					--------------------------
 *	
 *							p = 1		means that adjacently preceding block is in use
 *							t = 1		means that this block is in use
 *	
 *					--------------------------
 *					| 0 |    size    | 0 | 0 |			block end tag
 *					--------------------------
 *	
 *	
 *					--------------------------
 *					|        size(-1)        |	/ \
 *					--------------------------	 |
 *					|     block start tag    |	 |
 *					--------------------------	 |
 *					|  prev available block  | --|
 *					--------------------------
 *					|  next available block  | --|
 *					--------------------------	 |
 *					|                        |	 |	available block (within its heap)
 *					|           ...          |	 |
 *					|                        |	\ /
 *					--------------------------
 *					|      block end tag     |
 *					--------------------------
 *					|        size(-1)        |
 *					--------------------------
 *	
 *	
 *					--------------------------
 *					|     block start tag    |
 *					--------------------------
 *					|                        |
 *					|                        |
 *					|                        |
 *					|                        |			unavailable block
 *					|          ...           |
 *					|                        |
 *					|                        |
 *					|                        |
 *					|                        |
 *					--------------------------
 *
 *	Pool Options
 *	------------
 *
 *	The following low-level options may be set for each memory pool object
 *	after __init_pool_obj has been called for that object (see pool_alloc.h):
 *	
 *	  sys_alloc_func points to a function that is called to acquire memory
 *	  blocks from the host environment. It takes an unsigned long as the
 *	  number of bytes needed and returns either NULL or a pointer to a newly
 *	  allocated block. If sys_alloc_func is NULL, then no attempt is made
 *	  to automatically acquire memory from the host environment. However, it
 *	  is still possible to supply the pool with memory on a manual basis
 *	  via __pool_preassign.
 *	  
 *	  sys_free_func points to a free function that is called to return
 *	  memory blocks to the host environment. This happens when a call to
 *	  __pool_free causes a heap to become entirely free. If sys_free_func
 *		is NULL, then no attempt is made to return memory to the host
 *		environment.
 *	  
 *	  A request for a block sized greater than 'min_heap_size' is always
 *	  allocated a heap all its own (with the exception noted below).
 *	  
 *	  Whenever a smaller request can't be satisfied from the list of
 *	  available blocks, a new heap of size 'min_heap_size' is created and
 *	  the requested block is allocated from it.
 *
 *		Normally, a new heap would be created for a request greater than
 *		min_heap_size bytes even if a block of the requested size is
 *		already available. The 'always_search_first' flags forces
 *		__pool_alloc to check first and this flag is set by __pool_preallocate
 *		to enable you to allocate whatever you preallocate.
 *	
 *	Modification History
 *	--------------------
 *
 *	24-Feb-95 JFH  First code release.
 *	10-Mar-95 JFH  Removed hash buckets. Was actually slowing things down.
 *  14-Mar-95 JFH  Fixed bug in merge_block causing it to call sys_free at
 *                 inappropriate times. Also added missing 'return' at end
 *                 of 'calloc'.
 *	20-Mar-95 JFH  Transformed code from an implementation of malloc, free,
 *                 etc. into a more generic tool for manipulating memory pool
 *                 objects. Needless to say, one such object will be used by
 *                 malloc, free, etc.
 *	24-Mar-95 JFH  Rearranged code so that certain compile-time options
 *                 became run-time options. Specifically, sys_alloc and
 *                 sys_free are accessed through pointers that may be
 *                 overridden. min_heap_size is no longer constant. A new
 *                 option named 'new_heap_size' was added. This now becomes
 *                 congruent with '_newpoolsize' and '_newnonptrmax' in
 *                 NewXX.cp.
 *  27-Mar-95 JFH  Added capability to pre-allocate or pre-assign memory to
 *                 a given pool.
 *	14-Aug-95 JFH  Pulled new_heap_size out of the equation. The min/new
 *								 dichotomy was confusing at best and led to some interesting
 *								 pathological situations in situations, particularly
 *								 involving __pool_preallocate. Added an option flag,
 *								 always_search_first, that tells __pool_alloc to always search
 *								 the free list, even if the requested block is greater than
 *								 min_heap_size bytes. This flag is set by __pool_preallocate.
 *	12-Oct-95 JFH  Moved #include of <Memory.h> to pool_alloc.mac.c
 *	17-Oct-95 JFH  Changed names to protect the innocent
 *	13-Dec-95 JFH  Fixed bug in __pool_alloc where a size within 3 of ULONG_MAX
 *								 would overflow to a more reasonable value when adding overhead.
 *								 Overflow checking now performed.
 *  22-Jan-96 JFH  Added casts from (void *) for C++ compatibility.
 *	29-Apr-96 JFH  Merged Win32 changes in.
 *						CTV
 */

#pragma mark #includes

//#define NDEBUG
#include <assert.h>
#include <limits.h>
#include "pool_alloc.h"

/*
 *	'alignment' is a mask used to ensure that blocks allocated always have
 *	sizes that are multiples of a given power-of-two, in this case: four. Other
 *	values are possible, but for internal reasons the alignment factor must be
 *	a multiple of four and must also be a multiple of sizeof(long).
 */

#if __POWERPC__ || __INTEL__

#define alignment				(8L-1L)			/* 8-byte alignment */

#else

#define alignment				(4L-1L)			/* 4-byte alignment */

#endif

/*
 *	'block_header' and 'block_trailer' define the structure of the beginning
 *	and end respectively of a free-block. The number of bytes that may lie
 *	between them is implied by the size portion of the header tag as well as
 *	the size in the trailer (the size is actually the number of bytes in the
 *	block including the header and trailer).
 */
 
 /*

typedef signed long	tag_word;

typedef struct block_header {
	tag_word							tag;
	struct block_header *	prev;
	struct block_header *	next;
} block_header;

*/

typedef signed long	size_word;

typedef size_word	block_trailer;

#define block_overhead	(sizeof(block_header) + sizeof(block_trailer))

#define heap_overhead	(2*sizeof(tag_word))	/* for guard words */

/*
 *	A free-block list is kept as a circular doubly linked list. The head of the
 *	list looks like any other list element except that it's size is nominally
 *	zero so that it is never a candidate for use. The 'rover' field preceding
 *	the header is a "finger" kept in the list at the last point where a block
 *	was taken from and is the point at which the next search for a suitable
 *	free-block begins. Using the 'rover' facilitates a more balanced
 *	distribution of block sizes in the list (see Knuth for details).
 */
 
 /*

typedef struct list_header {
	block_header *	rover;
	block_header		header;	
} list_header;

typedef struct mem_pool_obj {
	list_header	free_list;
} mem_pool_obj;

*/

/*
 *	The following are masks for manipulating the 'in-use' bits borrowed from
 *	the two low-order bits of the block size (this is why size must be a
 *	multiple of four). Collectively, the size and the in-use bits are
 *	referred to as a 'tag'.
 */

#define this_in_use		1L
#define prev_in_use		2L
#define in_use_flags	(prev_in_use | this_in_use)
#define this_size			(~in_use_flags)

/*
 *	The following macros facilitate the access of fields whose addresses must
 *	be computed at runtime. 'next_header' takes a pointer to a block and its
 *	size (the size is not derived by the macro from the block itself because
 *	the size may already be more conveniently available in a local variable)
 *	and yields a pointer to the next sequential block in memory (not
 *	necessarily the same as block->next). 'prev_header' does the same to yield
 *	a pointer to the previous sequential block in memory. 'prev_size' yields a
 *	pointer to the trailer of the previous sequential block in memory.
 */

#define next_header(block, block_size)	((block_header *)  (((char *) block) + block_size))
#define prev_size(block)								((block_trailer *) (((char *) block) - sizeof(block_trailer)))
#define prev_header(block, block_size)	((block_header *)  (((char *) block) - block_size))

/* internal functions */

enum {
	block_used      = 1,
	block_in_list   = 2,
	block_dont_care = 4
};

static block_header *	init_new_heap(char * new_heap, mem_size size);
static void						list_insert(list_header * free_list, block_header * block);
static void						list_remove(list_header * free_list, block_header * block);
static block_header *	list_search(list_header * free_list, mem_size size_needed);
static int						block_ok(list_header * free_list, block_header * block, int expected_status);
static mem_size				split_block(list_header * free_list, block_header * block, mem_size new_size);
static block_header *	merge_block(list_header * free_list, block_header * block);
static mem_size				grow_block(list_header * free_list, block_header * block, mem_size new_size);

/*
 *	__init_pool_obj
 *	
 *	Initializes the free-list(s) and sets the default options.
 */

void __init_pool_obj(mem_pool_obj * pool_obj)
{
	list_header *	p = &pool_obj->free_list;
	
	assert(sizeof(mem_size) == sizeof(long));											/* make sure fields will be compatible  */
	assert(sizeof(long) >= 4);																		/* make sure fields will be big enough  */
	assert(((alignment + 1) & 3) == 0);														/* make sure alignment is multiple of 4 */
	assert(((alignment + 1) & (sizeof(long) - 1)) == 0);					/* make sure alignment is multiple of sizeof(long) */

	p->header.tag  = 0;
	p->rover       =
	p->header.prev =
	p->header.next = &p->header;
	
	pool_obj->options.sys_alloc_func = &__sys_alloc;
	pool_obj->options.sys_free_func  = &__sys_free;
	
	pool_obj->options.min_heap_size       = 32768L;
	pool_obj->options.always_search_first =     0 ;
}

/*
 *	__pool_preallocate
 *	
 *		Pre-fetches enough memory from the host environment to subsequently
 *		satisfy a request for 'size' bytes without having to go to the host
 *		environment again for more. The amount pre-fetched will be at least
 *		'size' bytes, but possibly more. More precise control over the amount
 *		pre-fetched may be obtained by fiddling with 'min_heap_size'.
 *		
 *		The pre-fetch is literally done by calling __pool_alloc. The block
 *		returned by __pool_alloc is immediately freed with sys_free_func
 *		temporarily cleared so that it is kept on the free-list instead of
 *		being returned to the host environment.
 *
 *		Normally, you would not be able to allocate more than min_heap_size
 *		bytes out of a preallocated area. This is because __pool_alloc's default
 *		behavior is to assume that a request for more than min_heap_size bytes
 *		is to be satisfied directly from system memory. __pool_preallocate sets
 *		the always_search_first option flag to ensure that __pool_alloc consults
 *		its free list before asking for system memory.
 */

int __pool_preallocate(mem_pool_obj * pool_obj, mem_size size)
{
	sys_free_ptr	save_sys_free;
	char *				block;
	
	save_sys_free = pool_obj->options.sys_free_func;
	pool_obj->options.sys_free_func = 0;
	
	block = (char *) __pool_alloc(pool_obj, size);
	
	__pool_free(pool_obj, block);
	
	pool_obj->options.sys_free_func = save_sys_free;
	
	pool_obj->options.always_search_first = 1;
	
	return(block != 0);
}

/*
 *	__pool_preassign
 *	
 *		Pre-assigns a block of memory to be a given pool's one and only heap.
 *		Specifically, it incorporates the given block into the pool's free-list
 *		and clears the sys_alloc_func and sys_free_func hooks.
 *		
 *		This capability is provided for situations where you will want memory
 *		and host environment memory will be potentially unavailable or
 *		inaccessible. Note that this may include interrupt-service time,
 *		however care must be taken to ensure that nothing is done to a pool
 *		object that is already in the middle of some pool function when the
 *		interrupt occurred.
 *		
 *		It is intended that this routine be called immediately after calling
 *		__init_pool_obj and that the pool's options remain undisturbed
 *		thereafter. Try anything more fancy at your own risk.
 */
 
void __pool_preassign(mem_pool_obj * pool_obj, void * ptr, mem_size size)
{
	list_header  *	free_list = &pool_obj->free_list;
	
	if (!ptr || size < (heap_overhead + block_overhead))
		return;
	
	list_insert(free_list, init_new_heap((char *) ptr, size));
	
	pool_obj->options.sys_alloc_func = 0;
	pool_obj->options.sys_free_func = 0;
}

/*
 *	__pool_alloc
 *	
 *		Allocates a block of memory of the given size (or possibly a bit more for
 *		alignment, etc.). Returns a pointer to the allocated block or a null
 *		pointer if the block couldn't be allocated.
 */

void * __pool_alloc(mem_pool_obj * pool_obj, mem_size size)
{
	list_header  *	free_list = &pool_obj->free_list;
	char				 *	new_heap;
	block_header *	block;
	mem_size				heap_size;
	
	if (size <= 0 || size > ULONG_MAX - (sizeof(tag_word) + alignment))
		return(0);
	
	size += sizeof(tag_word);
	size  = (size + alignment) & ~alignment;
	
	if (size >= pool_obj->options.min_heap_size
				&&  pool_obj->options.sys_alloc_func
				&& !pool_obj->options.always_search_first)
	{
		heap_size = size + heap_overhead;
		
		new_heap = (char *) (*pool_obj->options.sys_alloc_func)(heap_size);
		
		block = init_new_heap(new_heap, heap_size);
		
		if (!block)
			return(0);
		
		size  = block->tag & this_size;
	}
	else
	{	
		if (size < block_overhead)
			size = block_overhead;
		
		block = list_search(free_list, size);
		
		if (block)
			list_remove(free_list, block);
		else
		{
			if (pool_obj->options.sys_alloc_func)
			{
				if (size <= pool_obj->options.min_heap_size)
					heap_size = pool_obj->options.min_heap_size + heap_overhead;
				else
					heap_size = size + heap_overhead;
				
				new_heap = (char *) (*pool_obj->options.sys_alloc_func)(heap_size);
				
				block = init_new_heap(new_heap, heap_size);
			}
			
			if (!block)
				return(0);
		}
		
		size = split_block(free_list, block, size);
	}
	
	block->tag |= this_in_use;
	
	next_header(block, size)->tag |= prev_in_use;
	
	return((char *) block + sizeof(tag_word));
}

/*
 *	__pool_alloc_clear
 *
 *	Essentially does a __pool_alloc(size) and then clears all the bytes in
 *	the allocated block.
 */

void * __pool_alloc_clear(mem_pool_obj * pool_obj, mem_size size)
{
	long *	ptr;
	long *	p;
	
	size = (size + alignment) & ~alignment;
	
	ptr = (long *) __pool_alloc(pool_obj, size);
	
	if (!ptr)
		return(0);

#if !__POWERPC__
	
	for (size = (size / sizeof(long)) + 1, p = ptr; --size;)
		*p++ = 0;

#else
	
	for (size = (size / sizeof(long)) + 1, p = ptr - 1; --size;)
		*++p = 0;

#endif
	
	return(ptr);
}

/*
 *	__pool_realloc
 *
 *	The size of the given block is changed to the new given size. If the new
 *	size is less than the old size, the excess bytes are returned to the
 *	free-list (unless there are not enough excess to meet minimum block size
 *	requirements) and the original pointer is returned.
 *	
 *	If the new size is greater than the old size and if the memory following
 *	the block is free and there is enough of it, then the block is enlarged
 *	using the free bytes. Otherwise, a new block of the given size is
 *	allocated and the contents of the old block are copied into it before the
 *	old block is freed. A pointer to either the enlarged old block or the
 *	newly allocated block is returned, unless more bytes were not available at
 *	all, in which case a null pointer is returned.
 *	
 *	If 'ptr' is null, then we return 'malloc(size)'.
 *	
 *	If 'size' is zero and 'ptr' is not null, we 'free(ptr)' and return a null
 *	pointer.
 */

void * __pool_realloc(mem_pool_obj * pool_obj, void * ptr, mem_size size)
{
	list_header  *	free_list = &pool_obj->free_list;
	block_header *	block;
	mem_size				block_size, i;
	char *					new_block;
	long *					p;
	long *					q;
	
	if (!ptr)
		return(__pool_alloc(pool_obj, size));
	
	if (!size)
	{
		__pool_free(pool_obj, ptr);
		return(0);
	}
	
	block = (block_header *) ((char *) ptr - sizeof(tag_word));

	assert(block_ok(free_list, block, block_used | !block_in_list));

	size = size + sizeof(tag_word);
	size = (size + alignment) & ~alignment;

	if (size < block_overhead)
		size = block_overhead;
	
	block_size = block->tag & this_size;
	
	if (size == block_size)
		return(ptr);
	
	if (size < block_size)
	{
		split_block(free_list, block, size);
		return(ptr);
	}
	
	block_size = grow_block(free_list, block, size);
	
	if (block_size >= size)
		return(ptr);
	
	new_block = (char *) __pool_alloc(pool_obj, size - sizeof(tag_word));
	
	if (!new_block)
		return(0);

#if !__POWERPC__
	
	for (i = block_size / sizeof(long), p = (long *) ptr, q = (long *) new_block; --i;)
		*q++ = *p++;

#else
	
	for (i = block_size / sizeof(long), p = (long *) ptr - 1, q = (long *) new_block - 1; --i;)
		*++q = *++p;

#endif
	
	__pool_free(pool_obj, ptr);
	
	return(new_block);
}

/*
 *	__pool_free
 *	
 *	The block pointed to by the given pointer is returned to the appropriate
 *	list of free-blocks. If the immediately preceding or following blocks of
 *	memory are currently free, they are merged with this block. If, as a
 *	result of this merging, an entire heap becomes free, it is returned to the
 *	system (maybe).
 */

void __pool_free(mem_pool_obj * pool_obj, void * ptr)
{
	list_header  *	free_list = &pool_obj->free_list;
	block_header *	block;
	block_header *	other_block;
	size_word				block_size;
	
	if (!ptr)
		return;
	
	block = (block_header *) ((char *) ptr - sizeof(tag_word));

	assert(block_ok(free_list, block, block_used | !block_in_list));
	
	block->tag &= ~this_in_use;
	
	block_size = block->tag & this_size;
	
	other_block = next_header(block, block_size);
	
	other_block->tag &= ~prev_in_use;
	
	*prev_size(other_block) = block_size;
	
	block = merge_block(free_list, block);
	
	block_size = block->tag & this_size;
	
	other_block = next_header(block, block_size);
	
	if (	pool_obj->options.sys_free_func	&&
				!(block->tag & prev_in_use)			&&
				* prev_size(block) < 0					&&
				other_block->tag < 0								)
	{
		(*pool_obj->options.sys_free_func)((char *) block - sizeof(tag_word));
	}
	else
		list_insert(free_list, block);
}

/*
 *	init_new_heap
 *	
 *	Takes a heap of memory and initializes the guard words and the header and
 *	trailer to make the heap look like one large block and returns a pointer
 *	to the header.
 */

static block_header * init_new_heap(char * new_heap, mem_size size)
{
	char *	p;
	
	if (!new_heap)
		return(0);
	
	p = new_heap;
	
	size -= heap_overhead;
	
	assert(size >= block_overhead);
	
	* (tag_word *) p = -1 & this_size;	p += sizeof(tag_word);				/* guard tag */
	* (tag_word *) p = size;						p += size - sizeof(tag_word);	/* block header tag */
	* (tag_word *) p = size;						p += sizeof(tag_word);				/* block trailer tag */
	* (tag_word *) p = -1 & this_size;																/* guard tag */
	
	return((block_header *) (new_heap + sizeof(tag_word)));
}

/*
 *	list_insert
 *	
 *	Inserts the given block in the appropriate free-list (based upon the size
 *	of the given block).
 */

static void list_insert(list_header * free_list, block_header * block)
{
	assert(block_ok(free_list, block, !block_used | !block_in_list));
	
	block->prev = &free_list->header;
	block->next = free_list->header.next;
	
	free_list->header.next->prev = block;
	free_list->header.next       = block;
}

/*
 *	list_remove
 *	
 *	Removes the given block from the free-list it is linked to. Normally, since
 *	this is a doubly linked circular list, it wouldn't be necessary to refer
 *	explicitly to the list's header (because it looks like any other block).
 *	However, we need to refer to the 'rover' in the header and, if it points
 *	to the block we're about to remove, we first advance the 'rover' to the
 *	next block in the list.
 */

static void list_remove(list_header * free_list, block_header * block)
{
	assert(block_ok(free_list, block, !block_used | block_in_list));
	
	if (block == free_list->rover)
		free_list->rover = free_list->rover->next;
	
	block->next->prev = block->prev;
	block->prev->next = block->next;
}

/*
 *	list_search
 *	
 *	Given a requested number of bytes, search the appropriate free-list for an
 *	available block at least that large. If none is found, search the
 *	remaining lists in the hash table that might contain even larger blocks.
 *	Return a pointer to the first suitable block found, if any.
 */

static block_header * list_search(list_header * free_list, mem_size size_needed)
{
	block_header *	curr_block;
	block_header *	last_block;
	
	last_block = curr_block = free_list->rover;
	
	do
	{
		if ((curr_block->tag & this_size) >= size_needed)
		{
			free_list->rover = curr_block;
			return(curr_block);
		}
			
		curr_block = curr_block->next;
	}
	while (curr_block != last_block);
	
	return(0);
}

/*
 *	block_ok
 *	
 *	Makes several assertions to perform various sanity checks on a given block.
 *	Furthermore, this function is only ever called from within an assertion.
 *	Its purpose, of course, is to ensure data integrity during development.
 *	Items checked for include:
 *	
 *		o	We check that the given block pointer is non-null and points to what
 *			appears to be a block of at least 'block_overhead' bytes (this guards
 *			against negative sizes as well).
 *			
 *		o	If we don't care what the block's status is, we assume its current
 *			in-use status and that it is not in a free-list. (This is a special
 *			case for split_block because we may split a free block that is bigger
 *			than we need or we may split a used block passed to realloc that we
 *			need to shrink. In both cases, the block to be split will not be in a
 *			free-list.)
 *			
 *		o	If the block's expected in-use status is that it is used, we confirm
 *			that the in-use flags in the block's header and in the following
 *			block's header are set appropriately. If the block's expected in-use
 *			status is that it is free, we similarly check the flags as well as
 *			ensuring that the size indicated by both the header and the trailer
 *			match.
 *			
 *		o	We search the free-list appropriate to the block's size to see if the
 *			block is appropriately present or not in the list.
 *	
 *	If we get past all the assertions here, we return true to calling assertion.
 */

static int block_ok(list_header * free_list, block_header * block, int expected_status)
{
	block_header *	curr_block;
	block_header *	last_block;
	
	assert(block != 0 && (block->tag & this_size) >= block_overhead);
	
	if (expected_status & block_dont_care)
		if (block->tag & this_in_use)
			expected_status = block_used | !block_in_list;
		else
			expected_status = !block_used | !block_in_list;
	
	if (expected_status & block_used)
		assert((block->tag & this_in_use) && (next_header(block, (block->tag & this_size))->tag & prev_in_use));
	else
	{
		assert((!(block->tag & this_in_use)) && (!(next_header(block, (block->tag & this_size))->tag & prev_in_use)));
		assert((block->tag & this_size) == * prev_size(next_header(block, (block->tag & this_size))));
	}
	
	last_block = curr_block = &free_list->header;
	
	do
	{
		if (curr_block == block)
			break;
			
		curr_block = curr_block->next;
	}
	while (curr_block != last_block);
	
	if (expected_status & block_in_list)
		assert(curr_block == block);
	else
		assert(curr_block != block);
	
	return(1);
}

/*
 *	split_block
 *	
 *	Given a block and a desired size, reduces the size of the block to that
 *	size while preserving alignment and minimum block size requirements.
 *	Excess bytes remaining are dealt with in one of two ways. If the block
 *	following the split block is free, then the excess bytes are merged into
 *	that block. Otherwise, they are returned to the appropriate free-list,
 *	unless they don't meet minimum block size requirements, in which case no
 *	reduction at all is made. Returns the final size of the block, which may
 *	be unchanged.
 */

static mem_size split_block(list_header * free_list, block_header * block, mem_size new_size)
{
	tag_word				block_tag, other_tag;
	mem_size				block_size, split_size, other_size;
	block_header *	split_block;
	block_header *	other_block;
	
	assert(block_ok(free_list, block, block_dont_care));
	
	new_size = (new_size + alignment) & ~alignment;
	
	block_tag = block->tag;
	
	block_size = block_tag & this_size;
	
	split_size = block_size - new_size;
	
	other_block = next_header(block, block_size);
	
	other_tag = other_block->tag;
	
	if (other_tag >= 0 && !(other_tag & this_in_use))
	{
		list_remove(free_list, other_block);
		
		other_size = other_tag & this_size;
		
		split_size += other_size;
		
		other_block = next_header(other_block, other_size);
	}
	else
		if (split_size < block_overhead)
			return(block_size);
	
	block->tag = new_size | (block_tag & in_use_flags);
	
	split_block = next_header(block, new_size);
	
	if (!(block_tag & this_in_use))
		* prev_size(split_block) = new_size;
	
	split_block->tag = split_size | ((block_tag & this_in_use) ? prev_in_use : 0);
	
	* prev_size(other_block) = split_size;
	
	other_block->tag &= ~prev_in_use;
	
	list_insert(free_list, split_block);
	
	return(new_size);
}

/*
 *	merge_block
 *	
 *	Inspects the immediately following and preceding blocks in memory. If
 *	either or both are free, they are merged with the given block. A pointer
 *	to the final merged block is returned.
 */

static block_header * merge_block(list_header * free_list, block_header * block)
{
	block_header *	other_block;
	tag_word				other_tag;
	size_word				size, other_size;

	assert(block_ok(free_list, block, !block_used | !block_in_list));
	
	size = block->tag & this_size;
	
	other_block = next_header(block, size);
	
	other_tag = other_block->tag;
	
	other_size = other_tag & this_size;
	
	if (other_tag >= 0 && !(other_tag & this_in_use))
	{
		list_remove(free_list, other_block);
		
		size += other_size;
		
		block->tag = size | (block->tag & in_use_flags);
	}
	
	other_size = * prev_size(block);
	
	if ((block->tag & prev_in_use) || other_size < 0)
		* prev_size(next_header(block, size)) = size;
	else
	{
		other_block = prev_header(block, other_size);
		
		list_remove(free_list, other_block);
		
		other_size += size;
		
		other_block->tag = other_size | (other_block->tag & in_use_flags);
		
		* prev_size(next_header(other_block, other_size)) = other_size;
		
		block = other_block;
	}
	
	return(block);
}

/*
 *	grow_block
 *	
 *	Similar to 'merge_block', except that in this case we are trying to merge
 *	an in-use block with a free block in order to make the former larger (in
 *	response to a realloc call). If the subsequently adjacent block is free
 *	and large enough, we merge the blocks and split off any excess (assuming
 *	the excess is enough to meet minimum block size requirements) and return
 *	it to the appropriate free-list. We return the final size of the block to
 *	the caller, which may be unchanged.
 */

static mem_size grow_block(list_header * free_list, block_header * block, mem_size new_size)
{
	block_header *	other_block;
	tag_word				block_tag, other_tag;
	size_word				size, other_size;

	assert(block_ok(free_list, block, block_used | !block_in_list));
	
	block_tag = block->tag;
	
	size = block_tag & this_size;
	
	other_block = next_header(block, size);
	
	other_tag = other_block->tag;
	
	if (other_tag < 0 || (other_tag & this_in_use))
		return(size);
	
	other_size = other_tag & this_size;
	
	if (size + other_size < new_size)
		return(size);
	
	list_remove(free_list, other_block);
	
	size += other_size;
	
	block->tag = size | (block_tag & in_use_flags);
	
	next_header(block, size)->tag |= prev_in_use;
	
	return(split_block(free_list, block, new_size));
}
