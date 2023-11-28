/*
	re-written version of the original mcheck.c because the old version
	just plain sucked and didn't work.

	dbg@be.com

	Modifications by sbarta@be.com.  Added the purgatory and heap lists,
	checking for modification when being freed from the purgatory list,
	full consistency checks every so often, debugging levels, and storage
	of allocation call stacks in the block header.
  
	The benaphore code is from nukernel/lock.h
	
	jeff: Added hooks for checking every memory access when -fcheck-memory-usage
	is specified.
*/   

#ifndef _MALLOC_INTERNAL
#define _MALLOC_INTERNAL
	#include "malloc.h"
	#include "malloc_priv.h"
#endif

#include <Debug.h>
#include <stdio.h>
#include <stdlib.h>
#include "avl_tree.h"

#define PAD_SIZE 1		 	/* number of ints to use as padding */
#define CALLSTACK_DEPTH 7	/* number of levels of call stack to record for each block */

#define FREEFLOOD    0x57
#define MALLOCFLOOD  0x51
#define REALLOCFLOOD 0x53
#define PURGFLOOD	 0x55
#define ALIGNFLOOD	 0x59

#define HEAD_PAD_OVERHEAD(alignment) (sizeof(struct hdr)+(sizeof(struct hdr)%(alignment)))
#define TAIL_PAD_OVERHEAD()			 (sizeof(tail_pad)+extra_padding)
#define PAD_OVERHEAD(alignment) (HEAD_PAD_OVERHEAD(alignment) + TAIL_PAD_OVERHEAD())
#define USER_DATA_START(hdr) ((uint) hdr + sizeof(struct hdr))
#define USER_DATA_END(hdr) (USER_DATA_START(hdr) + hdr->size - 1)

/* some utilities */
typedef struct lock {
	sem_id		s;
	long		c;
} lock;

#define MAX_READERS 0x5ffffffful

typedef struct rw_lock {
	int32 count;
	sem_id read_wait_sem;
	sem_id write_wait_sem;
	lock write_lock;
} rw_lock; 

struct hdr {    /* we always want this to be a multiple of 8 or 16 */
	int32 pre_pad[PAD_SIZE];
	struct avl_node tree_node;		/* Note: the offset of this within the structure
										is hardcoded in chkr_check_addr. */
	int32 _dummy;					/* node is odd size, round up to multiple of 8 */
	ssize_t size;
	ssize_t hdr_off;				/* offset from start of heap chunk to start of hdr */
	struct hdr *next_block;
	struct hdr *prev_block;
	size_t alloc_call_chain[CALLSTACK_DEPTH];
	thread_id alloc_thread;
	size_t free_call_chain[CALLSTACK_DEPTH];
	thread_id free_thread;
	int32 post_pad[PAD_SIZE];
};

/* these numbers are (literally) random 32 bit values */
static int32 pre_pad[PAD_SIZE]  = { 0x3975237f };
static int32 post_pad[PAD_SIZE] = { 0x792353f7 };
static int32 tail_pad[PAD_SIZE] = { 0x92753777 };
static int32 free_pad[PAD_SIZE] = { 0xc3e5b8f9 };

static struct avl_tree alloced_block_tree;		/* for quick lookup of allocated blocks */
static uint lowest_heap_address = 0xfffffffful;
static uint highest_heap_address = 0;
static struct hdr *heap_head = 0;
static struct hdr *purgatory_head = 0;
static struct hdr *purgatory_tail = 0;
static const struct hdr *bad_hdr = 0;

static int mcheck_used = 0;
static int extra_padding = 0;
static int ignore_stack = 0;

static rw_lock heap_lock;
static lock error_lock;

static unsigned int purgatory_count = 0;		/* The current number of blocks in purgatory. */
static unsigned int check_count = 0;			/* The number of times malloc/realloc/free have been
												   called. */ 

static unsigned int purgatory_size = 1000;		/* Number of old blocks to keep in purgatory.
										   		   When it fills up, the oldest blocks are freed first. */
static unsigned int check_frequency = 1000;	/* Do a full heap/purgatory integrity check every
										   	   [check_frequency] number of calls to
											   malloc/realloc/free. */
static unsigned int check_level = 1;		/* 1 = trash new/freed blocks, add padding, check
												   padding integrity on free
										  	   5 = when a block is freed, trash it, place it in a
											       FIFO purgatory queue, and check its integrity when
											       freed to see if it has been written to.
										  	   10 = every [check_frequency] calls to malloc/realloc/free,
											        integrity-check the entire list of heap and
											   		purgatory blocks.
											   15 = Use -fcheck-memory-usage compiler hooks to check every
											        memory access
											   20 = Used by electric fence. */
											

// This code to grab the caller's address off of the stack comes from
// Pavel's leak checker.
#if __POWERPC__
static __asm unsigned long * get_caller_frame();

static __asm unsigned long *
get_caller_frame ()
{
	lwz     r3, 0 (r1)
	blr
}

#endif

#define bogus_addr(x)  ((x) < 0x80000000)

static unsigned long
GetCallerAddress(int level)
{
#if __INTEL__ || __arm__	/* FIXME: Is this right? */
	ulong fp = 0, nfp, ret = 0;

	if (ignore_stack)
		return 0;
		
	level += 2;
	
	fp = (ulong)get_stack_frame();
	if (bogus_addr(fp))
		return 0;
	nfp = *(ulong *)fp;
	while (nfp && --level > 0) {
		if (bogus_addr(fp))
			return 0;
		nfp = *(ulong *)fp;
		ret = *(ulong *)(fp + 4);
		if (bogus_addr(ret))
			break;
		fp = nfp;
	}

	return ret;
#else
	unsigned long *cf = get_caller_frame();
	unsigned long ret = 0;
	
	if (ignore_stack)
		return 0;
		
	level += 1;
	
	while (cf && --level > 0) {
		ret = cf[2];
		if (ret < 0x80000000 || ret > 0xfc000000)
			break;
		cf = (unsigned long *)*cf;
	}

	return ret;
#endif
}

inline int new_lock(lock *l, const char *name)
{
	l->c = 1;
	l->s = create_sem(0, name);
	if (l->s <= 0)
		return l->s;
	return 0;
}

inline void free_lock(lock *l)
{
	delete_sem(l->s);
}

#define	LOCK(l)		if (atomic_add(&l.c, -1) <= 0) acquire_sem(l.s);
#define	UNLOCK(l)	if (atomic_add(&l.c, 1) < 0) release_sem(l.s);

inline void new_rw_lock(rw_lock *lock)
{
	lock->count = 0;
	lock->read_wait_sem = create_sem(0, "reader wait");
	lock->write_wait_sem = create_sem(0, "writer wait");
	new_lock(&lock->write_lock, "write lock");
}

inline void free_rw_lock(rw_lock *lock)
{
	delete_sem(lock->read_wait_sem);
	delete_sem(lock->write_wait_sem);
	free_lock(&lock->write_lock);
}

inline void lock_read(rw_lock *lock)
{
	if (atomic_add(&lock->count, 1) < 0)
		acquire_sem(lock->read_wait_sem);	/* wait on a writer */
}

inline void unlock_read(rw_lock *lock)
{
	if (atomic_add(&lock->count, -1) < 0)
		release_sem(lock->write_wait_sem);	/* signal writer */
}

inline void lock_write(rw_lock *lock)
{
	int reader_count;
	
	LOCK(lock->write_lock)
	reader_count = atomic_add(&lock->count, -MAX_READERS);
	if (reader_count > 0)
		acquire_sem_etc(lock->write_wait_sem, reader_count, 0, B_INFINITE_TIMEOUT);
}

inline void unlock_write(rw_lock *lock)
{
	int reader_count = atomic_add(&lock->count, MAX_READERS) + MAX_READERS;
	if (reader_count > 0)
		release_sem_etc(lock->read_wait_sem, reader_count, B_DO_NOT_RESCHEDULE);

	UNLOCK(lock->write_lock)
}

static void print_free_stack(const struct hdr *hdr, char *buffer)
{
	int i;
	char tmp[64];
	sprintf(tmp, "Free call chain (thid %ld): ", hdr->free_thread);
	strcat(buffer, tmp);
	for (i = 0; i < CALLSTACK_DEPTH; i++) {
		sprintf(tmp, " 0x%x", (unsigned int)hdr->free_call_chain[i]);
		strcat(buffer, tmp);
	}
	strcat(buffer, "\n");
}

static void print_alloc_stack(const struct hdr *hdr, char *buffer)
{
	int i;
	char tmp[64];
	sprintf(tmp, "Alloc call chain (thid %ld): ", hdr->alloc_thread);
	strcat(buffer, tmp);
	for (i = 0; i < CALLSTACK_DEPTH; i++) {
		sprintf(tmp, " 0x%x", (unsigned int)hdr->alloc_call_chain[i]);
		strcat(buffer, tmp);
	}
	strcat(buffer, "\n");
}

static enum mcheck_status checkheaphdr(const struct hdr *hdr,  malloc_funcs *mf)
{
	char *end;
	int i;

	if (memcmp(hdr->pre_pad, free_pad, sizeof(free_pad)) == 0) {
		LOCK(error_lock);
		bad_hdr = hdr;
		(*mf->abortfunc) (MCHECK_FREEDTWICE);
	}

	if (memcmp(&hdr->pre_pad, pre_pad, sizeof(pre_pad)) != 0 ||
		memcmp(&hdr->post_pad, post_pad, sizeof(post_pad)) != 0 ||
		hdr->size > 512 * 1024 * 1024) {
		LOCK(error_lock);
		bad_hdr = hdr;
		(*mf->abortfunc) (MCHECK_HEAD);
	}

	end = &((char *)&hdr[1])[hdr->size];
	
	for (i = 0; i <= extra_padding / sizeof(tail_pad); i++)
		if (memcmp(end + i * sizeof(tail_pad), tail_pad, sizeof(tail_pad)) != 0) {
			LOCK(error_lock);
			bad_hdr = hdr;
			(*mf->abortfunc) (MCHECK_TAIL);
		}

	if (hdr->size < 0 || hdr->size > 512 * 1024 * 1024) {
		LOCK(error_lock);
		bad_hdr = hdr;
		(*mf->abortfunc) (MCHECK_HEAD);
	}
	return MCHECK_OK;
}


static enum mcheck_status checkpurghdr(const struct hdr *hdr, malloc_funcs *mf)
{
	char *end;
	char *cur;
	int i;

	if (memcmp(&hdr->pre_pad, free_pad, sizeof(free_pad)) != 0 ||
		memcmp(&hdr->post_pad, post_pad, sizeof(post_pad)) != 0 ||
		hdr->size > 512 * 1024 * 1024) {
		LOCK(error_lock);
		bad_hdr = hdr;
		(*mf->abortfunc) (MCHECK_HEAD);
	}

	end = &((char *)&hdr[1])[hdr->size];
	
	for (i = 0; i <= extra_padding / sizeof(tail_pad); i++)
		if (memcmp(end + i * sizeof(tail_pad), tail_pad, sizeof(tail_pad)) != 0) {
			LOCK(error_lock);
			bad_hdr = hdr;
			(*mf->abortfunc) (MCHECK_TAIL);
		}

	for (cur = (char *)(hdr + 1); cur < end; cur++)
		if (*cur != PURGFLOOD) {
			LOCK(error_lock);
			bad_hdr = hdr;
			(*mf->abortfunc) (MCHECK_FREE);
			break;
		}
	return MCHECK_OK;
}


static void checkeverything(malloc_funcs *mf)
{
	const struct hdr *hdr;

	lock_read(&heap_lock);
	hdr = heap_head;
	while (hdr) {
		checkheaphdr(hdr, mf);
		hdr = hdr->next_block;
	}

	hdr = purgatory_head;
	while (hdr) {
		checkpurghdr(hdr, mf);
		hdr = hdr->next_block;
	}
	unlock_read(&heap_lock);
}


static void freehook(void *ptr, malloc_state *ms, malloc_funcs *mf)
{
	struct hdr *hdr;
	int i;

	if (check_level >= 10 && ++check_count % check_frequency == 0)
		checkeverything(mf);

	if (ptr != NULL) {
		hdr = ((struct hdr *) ptr) - 1;	  

		/* free() on a non-heap pointer is an error */
		if ((ptr < ms->sbrk_base) || (ptr >= ms->sbrk_base + ms->max_sbrked_mem))
		{
			char buf[64];
			sprintf(buf, "Attempt to free() non-heap pointer %p\n", ptr);
			debugger(buf);
			return;
		}

		if (check_level >= 5)
			lock_write(&heap_lock);

		checkheaphdr (hdr, mf);
		
		/* save who freed this block */
		for (i = 1; i <= CALLSTACK_DEPTH; i++)
			hdr->free_call_chain[i - 1] = GetCallerAddress(i);

		hdr->free_thread = find_thread(NULL);
		if (check_level == 15)
			avl_remove_node(&alloced_block_tree, &hdr->tree_node);

		memcpy(hdr->pre_pad, free_pad, sizeof(free_pad));
		
		if (check_level >= 5)
			memset(ptr, PURGFLOOD, hdr->size);
		else
			memset(ptr, FREEFLOOD, hdr->size);
		
		if (check_level >= 10) {
			if (hdr == heap_head)
				heap_head = hdr->next_block;
			if (hdr->prev_block)
				hdr->prev_block->next_block = hdr->next_block;
			if (hdr->next_block)
				hdr->next_block->prev_block = hdr->prev_block;
		}

		if (check_level >= 5) {
			hdr->prev_block = 0;
			hdr->next_block = purgatory_head;
			if (purgatory_head)
				purgatory_head->prev_block = hdr;
			if (purgatory_tail == 0)
				purgatory_tail = hdr;
			purgatory_head = hdr;
			purgatory_count++;
			if (purgatory_count > purgatory_size) {
				hdr = purgatory_tail;
				if (hdr->prev_block)
					hdr->prev_block->next_block = 0;
				purgatory_tail = hdr->prev_block;
				purgatory_count--;
				checkpurghdr (hdr, mf);
				memset((void *)(hdr + 1), FREEFLOOD, hdr->size);
			} else
				hdr = NULL;
				
			unlock_write(&heap_lock);
		}
	} else {
		hdr = NULL;
	}
  
  if (hdr) _free ((unsigned char *)hdr - hdr->hdr_off, ms, mf);
}


struct malloc_context
{
	// inputs
	size_t size;			// size of new block
	void *in_hdr;			// data already allocated
	malloc_state *ms;
	malloc_funcs *mf;
	uint8 flood;			// what to fill new block with
	void *old_addr;			// previous block from realloc()
	size_t old_size;		// size of previous block
	
	// output
	struct hdr *hdr;		// base hdr of new block
	bool locked;			// returning with heap locked?
};

static void* generic_mallochook(struct malloc_context* context)
{
	struct hdr *hdr;
	int i;

	context->locked = false;
	
	if (context->in_hdr) {
		context->hdr = context->in_hdr;
	}
	else {
		context->hdr = (struct hdr *) _malloc (PAD_OVERHEAD(8) + context->size,
											  context->ms, context->mf);
	}
	if (context->hdr == NULL)
		return NULL;
	
	hdr = context->hdr;
	
	memcpy(hdr->pre_pad, pre_pad, sizeof(pre_pad));
	hdr->size = context->size;
	hdr->hdr_off = 0;
 	memcpy(hdr->post_pad, post_pad, sizeof(post_pad));
	for (i = 1; i <= CALLSTACK_DEPTH; i++)
		hdr->alloc_call_chain[i - 1] = GetCallerAddress(i);

	hdr->alloc_thread = find_thread(NULL);
	if (context->old_addr != NULL) {
		size_t min = context->size < context->old_size ? context->size : context->old_size;
		memcpy((void*)(hdr + 1), context->old_addr, min);
		memset((char*)(hdr + 1) + min, context->flood, context->size-min);
	} else {
		memset((void*)(hdr + 1), context->flood, context->size);
	}
	
	for (i = 0; i <= extra_padding / sizeof(tail_pad); i++)
		memcpy((char*)(hdr+1) + context->size + i * sizeof(tail_pad), tail_pad, sizeof(tail_pad));

	if (check_level >= 10) {
		lock_write(&heap_lock);
		context->locked = true;
		hdr->prev_block = 0;
		hdr->next_block = heap_head;
		if (heap_head)
			heap_head->prev_block = hdr;
		heap_head = hdr;
		if (check_level == 15) {
			avl_add_node(&alloced_block_tree, &hdr->tree_node,
						 (uint) hdr,
						 (uint) hdr + hdr->size + PAD_OVERHEAD(8) - 1);

			/* Add a small fudge factor to the heap boundries to catch out of bounds accesses
			   for blocks that are near the boundries */
			if ((unsigned) hdr < lowest_heap_address)
				lowest_heap_address = (unsigned) hdr - 20;
			if ((unsigned) hdr + PAD_OVERHEAD(8) + hdr->size > highest_heap_address)
				highest_heap_address = (unsigned) hdr + PAD_OVERHEAD(8) + hdr->size + 20; 
		}

		if (!context->old_addr) {
			unlock_write(&heap_lock);
			context->locked = false;
		}
	}

	return (void *) (hdr + 1);
}

static void* mallochook(size_t size, malloc_state *ms, malloc_funcs *mf)
{
	struct malloc_context context;
	context.size = size;
	context.in_hdr = NULL;
	context.ms = ms;
	context.mf = mf;
	context.flood = MALLOCFLOOD;
	context.old_addr = NULL;
	context.old_size = 0;
	
	if (check_level >= 10 && ++check_count % check_frequency == 0)
		checkeverything(mf);

	return generic_mallochook(&context);
}


static void* memalignhook(size_t align, size_t size, malloc_state *ms, malloc_funcs *mf)
{
	struct hdr *hdr;
	unsigned char *chunk;
	unsigned char *data;
	struct malloc_context context;
	size_t need;

	if (check_level >= 10 && ++check_count % check_frequency == 0)
		checkeverything(mf);

	/* align must be a power of 2.
	 * according to the malloc comments, other values could cause corruption.
	 */
	if (~(align-1) & (align>>1)) {
		debugger("memalign(): alignment must be a power of 2");
	}

	context.size = size;
	context.in_hdr = NULL;
	context.ms = ms;
	context.mf = mf;
	context.flood = MALLOCFLOOD;
	context.old_addr = NULL;
	context.old_size = 0;

	/* malloc already returns 8-aligned blocks */
	if (align < 8) {
		return generic_mallochook(&context);
	}

	need = PAD_OVERHEAD(8) + align + size;
	chunk = (unsigned char *) _malloc (need, ms, mf);
	if (chunk == NULL)
		return NULL;
	
	data = chunk + HEAD_PAD_OVERHEAD(8);
	data = (unsigned char *)(((uint32)data + align-1) & ~(align-1));
	context.in_hdr = (struct hdr *)data - 1;

	generic_mallochook(&context);
	hdr = context.hdr;
	hdr->hdr_off = data - chunk - HEAD_PAD_OVERHEAD(8);

	memset(chunk, ALIGNFLOOD, hdr->hdr_off);
	memset(data + size + TAIL_PAD_OVERHEAD(), ALIGNFLOOD, align - hdr->hdr_off);

	return data;
}


static void* reallochook(void *ptr, size_t size, malloc_state *ms, malloc_funcs *mf)
{
	struct hdr *ohdr;
	struct hdr *nhdr;
	size_t osize;
	int i;

	if (check_level >= 10 && ++check_count % check_frequency == 0)
		checkeverything(mf);

	if (ptr != NULL) {
		ohdr = ((struct hdr*) ptr) - 1;	  
		checkheaphdr(ohdr, mf);
		osize = ohdr->size;
	} else {
		ohdr = NULL;
		osize = 0;
	}

	/* don't actually use realloc() for MALLOC_DEBUG >= 5
	 * so we can find stale pre-realloc pointers
	 *
	 * don't use regular realloc() for memaligned data,
	 * there's no pretty way to avoid possibly
	 * copying header data.
	 */
	if(check_level >= 5 || (ohdr && ohdr->hdr_off)) {
		struct malloc_context context;
		context.size = size;
		context.in_hdr = NULL;
		context.ms = ms;
		context.mf = mf;
		context.flood = REALLOCFLOOD;
		context.old_addr = ptr;
		context.old_size = osize;
	
		/* allocate a new chunk
		 */
		generic_mallochook(&context);
		if((nhdr=context.hdr) == NULL) {
			return NULL;
		}

		/* and purgify the old one
		 */

		if(ohdr != NULL) {

			if(!context.locked)
				lock_write(&heap_lock);

			if (check_level == 15 && ohdr != NULL)
				avl_remove_node(&alloced_block_tree, &ohdr->tree_node);

			/* save who freed this block */
			for (i = 1; i <= CALLSTACK_DEPTH; i++)
				ohdr->free_call_chain[i - 1] = GetCallerAddress(i);

			ohdr->free_thread = find_thread(NULL);
			memcpy(ohdr->pre_pad, free_pad, sizeof(free_pad));
			memset((void*)(ohdr+1), PURGFLOOD, ohdr->size);
		
			if (check_level >= 10) {
				if (ohdr == heap_head)
					heap_head = ohdr->next_block;
				if (ohdr->prev_block)
					ohdr->prev_block->next_block = ohdr->next_block;
				if (ohdr->next_block)
					ohdr->next_block->prev_block = ohdr->prev_block;
			}

			ohdr->prev_block = 0;
			ohdr->next_block = purgatory_head;
			if (purgatory_head)
				purgatory_head->prev_block = ohdr;
			if (purgatory_tail == 0)
				purgatory_tail = ohdr;
			purgatory_head = ohdr;
			purgatory_count++;
			if (purgatory_count > purgatory_size) {
				ohdr = purgatory_tail;
				if (ohdr->prev_block)
					ohdr->prev_block->next_block = 0;
				purgatory_tail = ohdr->prev_block;
				purgatory_count--;
				checkpurghdr (ohdr, mf);
				memset((void *)(ohdr + 1), FREEFLOOD, ohdr->size);
			} else
				ohdr = NULL;

			unlock_write(&heap_lock);

			if (ohdr) _free((unsigned char*)ohdr - ohdr->hdr_off, ms, mf);
			ohdr = NULL;
		}
	}
	else {
		/* use regular-school realloc()
		 */

		if (ohdr != NULL && size < osize) {
			memset((char*)(ohdr+1) + size, FREEFLOOD, osize - size);
		}

		nhdr = (struct hdr *) _realloc(ohdr, PAD_OVERHEAD(8) + size, ms, mf);
		if (nhdr == NULL) {
			return NULL;
		}

		nhdr->size = size;
		nhdr->hdr_off = 0;
		memcpy(nhdr->pre_pad, pre_pad, sizeof(pre_pad));
		memcpy(nhdr->post_pad, post_pad, sizeof(post_pad));

		for (i = 1; i <= CALLSTACK_DEPTH; i++)
			nhdr->alloc_call_chain[i - 1] = GetCallerAddress(i);

		nhdr->alloc_thread = find_thread(NULL);
		if (size > osize)
			memset((char *) (nhdr + 1) + osize, REALLOCFLOOD, size - osize);	

		for (i = 0; i <= extra_padding / sizeof(tail_pad); i++)
			memcpy((char*)(nhdr+1) + size + i * sizeof(tail_pad), tail_pad, sizeof(tail_pad));
	}

	return (void*)(nhdr+1);
}


static void mabort(enum mcheck_status status)
{
	char msg[256];
	*msg = 0;

	switch (status) {
	case MCHECK_OK:
		sprintf(msg, "Memory is consistent, library is buggy");
		break;
	case MCHECK_HEAD:
		sprintf(msg, "Memory has been clobbered before start of allocation (block start 0x%08x size %ld)\n", (unsigned int)&bad_hdr[1], bad_hdr ? bad_hdr->size : 0);
		break;
	case MCHECK_TAIL:
		sprintf(msg, "Memory has been clobbered beyond end of allocation (block start 0x%08x size %ld)\n", (unsigned int)&bad_hdr[1], bad_hdr ? bad_hdr->size : 0);
		break;
	case MCHECK_FREEDTWICE:
		sprintf(msg, "Block has been freed twice (block start 0x%08x size %ld)\n", (unsigned int)&bad_hdr[1], bad_hdr ? bad_hdr->size : 0);
		break;
	case MCHECK_FREE:
		sprintf(msg, "Block has been written to after being freed (block start 0x%08x size %ld)\n", (unsigned int)&bad_hdr[1], bad_hdr ? bad_hdr->size : 0);
		break;
	default:
		sprintf(msg, "Bogus mcheck_status, library is buggy");
		break;
	}

	if (bad_hdr) {
		print_alloc_stack (bad_hdr, msg);
		if (status == MCHECK_FREEDTWICE || status == MCHECK_FREE)
			print_free_stack(bad_hdr, msg);
	}

	bad_hdr = 0;
	UNLOCK(error_lock);

	debugger(msg);
}


int _mcheck(void (*func)(enum mcheck_status), malloc_state *ms, malloc_funcs *mf)
{
	mf->abortfunc = (func != NULL) ? func : &mabort;
	
	/* These hooks may not be safely inserted if malloc is already in use.  */
	if (!ms->malloc_initialized && !mcheck_used) {
		char *env;
		
		mf->old_free_hook    = mf->free_hook;
		mf->free_hook        = freehook;
		mf->old_malloc_hook  = mf->malloc_hook;
		mf->malloc_hook      = mallochook;
		mf->old_realloc_hook = mf->realloc_hook;
		mf->realloc_hook     = reallochook;
		mf->old_memalign_hook = mf->memalign_hook;
		mf->memalign_hook	 = memalignhook;
		mcheck_used = 1;
	
		new_rw_lock(&heap_lock);
		
		env = getenv("MALLOC_DEBUG");
		if (env) {
			check_level = atoi(env);
			if (check_level < 1)
				check_level = 1;
		}
		
		env = getenv("MALLOC_DEBUG_FREE_LIST_SIZE");
		if (env)
			purgatory_size = atoi(env);
			
		env = getenv("MALLOC_DEBUG_CHECK_FREQUENCY");
		if (env)
			check_frequency = atoi(env);
		
		env = getenv("MALLOC_DEBUG_NO_STACK_CRAWL");
		if (env)
			ignore_stack = 1;
		
		env = getenv("MALLOC_DEBUG_EXTRA_PADDING");
		if (env) {
			extra_padding = atoi(env);
			if (extra_padding > 0)
				extra_padding = ((int)((extra_padding + 1) / sizeof(tail_pad))) * sizeof(tail_pad);
		}
	
		avl_init_tree(&alloced_block_tree);
	}
	
	return mcheck_used ? 0 : -1;
}


enum mcheck_status _mprobe(void * ptr, malloc_funcs *mf)
{
  return mcheck_used ? checkheaphdr(((struct hdr *) ptr) - 1, mf) : MCHECK_DISABLED;
}

/*
 *	This is the hook that the compiler inserts calls to on memory accesses
 */
void chkr_check_addr(const void *address, size_t len, unsigned char right)
{
	const struct hdr *hdr = 0;
	struct avl_node *node;
	char error_msg[1024];
	
	if (check_level != 15) 
		return;

	/* skip this if it isn't in the heap */
	if ((uint) address < lowest_heap_address || (uint) address > highest_heap_address) 
		return;

	lock_read(&heap_lock);
	node = avl_find_node(&alloced_block_tree, (uint) address);
	if (node != 0)
		hdr = (struct hdr*)((uint) node - PAD_SIZE * sizeof(int));

	if (node == 0) {
		/*
		 * This is a bad access.  It's in the heap, but not in an allocated block.  See
		 * if it is in a block that's on the purgatory list.  I don't store purgatory
		 * blocks in the AVL tree, so do a linear lookup (speed isn't as super important
		 * here, we're about to bring up the debug window anyway).
		 */
		for (hdr = purgatory_head; hdr; hdr = hdr->next_block) {
			if ((uint) hdr <= (uint) address && (uint) hdr + hdr->size + PAD_OVERHEAD(8)
				>= (uint) address)
				break;	/* found it */
		}

		if (hdr != 0) {
			sprintf(error_msg, "Attempt to %s freed block at address %p (block start %p size %ld)\n",
				right == 1 ? "read from" : "write to", address, hdr + 1, hdr->size);
			print_alloc_stack(hdr, error_msg);	
			print_free_stack(hdr, error_msg);
		} else
			sprintf(error_msg, "Attempt to %s unallocated heap memory at address %p\n",
				right == 1 ? "read from" : "write to", address);
			
		debugger(error_msg);
	} else {
		/*
		 *	This is in an allocated block.  Make sure it is in the user data area, and not
		 *	in the headers or padding.
		 */
		if ((uint) address < USER_DATA_START(hdr)) {
			sprintf(error_msg, "Attempt to %s %ld bytes before start of allocation (block start 0x%08lx size %ld)\n",
				right == 1 ? "read" : "write", USER_DATA_START(hdr) - (uint) address,
				USER_DATA_START(hdr), hdr->size);
			print_alloc_stack(hdr, error_msg);
			debugger(error_msg);
		} else if ((uint) address + len - 1 > USER_DATA_END(hdr)) {
			sprintf(error_msg, "Attempt to %s %ld bytes after end of allocation  (block start 0x%08lx size %ld)\n",
				right == 1 ? "read" : "write", (uint) address + len - 1 - USER_DATA_END(hdr),
				USER_DATA_START(hdr), hdr->size);
			print_alloc_stack(hdr, error_msg);
			debugger(error_msg);
		}
	}

	unlock_read(&heap_lock);
}

void chkr_check_str(const char *ptr, unsigned char right)
{
	chkr_check_addr(ptr, 1, right);
}

/*
 *	Currently ignored, as this only checks heap blocks.  As some point, it would be useful
 *	to implement this.
 */
void chkr_set_right(const void *address, size_t len, unsigned char right)
{
}

void chkr_copy_bitmap (void* dest, void* src, size_t orig_len)
{
}

void chkr_check_exec(const void *ptr)
{
}

