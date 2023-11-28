
/*#if defined(NDEBUG)*/
/* #undef NDEBUG*/
/*#endif*/

#include "rt_alloc.h"
#include <OS.h>
#include <stdio.h>
#include <stdlib.h>
#include <Debug.h>
#include <MediaDefs.h>

rtm_pool * _rtm_pool = NULL;
static int is_debugging;

#define ALLOC_PAT 0x7f
#define FREE_PAT 0x7d
#define FRESH_PAT 0x7b


#if defined(__cplusplus)
extern "C" {
#endif
void rtm_dump_block(rtm_pool * pool, void * block);
//	this must match what's in trinity_p.h !!!
//fixme	We should move it all into RealtimeAlloc.h
status_t rtm_create_pool_etc(rtm_pool ** out_pool, size_t total_size, const char * name, uint32 flags, ...);
enum {
	B_RTM_FORCE_LOCKED = 0x1,
	B_RTM_FORCE_UNLOCKED = 0x2
};
#if defined(__cplusplus)
}
#endif
 
#define SETTING_FILE "/boot/home/config/settings/Media/MDefaultManager"


#if !NDEBUG
#define assert(x,y) do { if (!(x)) { fprintf(stderr, "%s:%d: thr %d: assertion failed: %s\n", __FILE__, __LINE__, find_thread(NULL), #x); printf y ; fflush(stdout); debugger("assertion failed"); } } while (0)
#else
#define assert(x,y) (void)0
#endif


typedef struct rtm_block rtm_block;

typedef struct rtm_free_list rtm_free_list;

struct rtm_free_list {
	rtm_free_list *
					next;
	rtm_free_list *
					prev;
};


struct rtm_pool {
	rtm_pool *	next;
	rtm_pool *	head;
	int32			lock_count;
	sem_id			lock_sem;
	area_id			area;
	int32			total_size;
	int32			free_size;
	int32			total_blocks;
	int32			free_blocks;
	rtm_free_list
					free_list;
	int32			min_free_size;
	int32			_reserved[4];
};

struct rtm_block {
	rtm_pool *	pool;
	rtm_block *
					prev;
	int32			phys_size;
	int32			log_size;	/* negative if free */
};
#define FREE_BLOCK -1
/* How much can we waste in a free block when allocating before we split into two blocks? */
#define PADDING_OK 48
/* Logical block sizes are rounded up to a multiple of PADSIZE (power of 2) */
#define PADSIZE 16

// split realloc'd blocks if less than R_S_P% of the block is used
#define REALLOC_SPLIT_PCT	50

static int valid_pool_ptr(rtm_pool * p)
{
	area_id id = area_for(p);
	if (id < 0) {
		char str[100];
		sprintf(str, "Address 0x%lx is not in an area!\n", p);
		debugger(str);
		return 0;
	}
	return 1;
}


static void
__init_rtm_pool(void)
{
	static long didit = 0;
	
	if (atomic_add(&didit, 1) == 0) {
		thread_info  ti;
		const char * ptr = getenv("MEDIA_LOCKED_MEMORY_SIZE");
		size_t size = ptr ? atoi(ptr) : 400000;
		rtm_pool    *temp;

		if (size < 128000) size = 128000;
		/*	Using more than 4 MB of locked heap? Hardly likely. */
		if (size > 4*1024*1024) size = 4*1024*1024;
		get_thread_info(find_thread(NULL), &ti);
		if (strcmp(ti.name, "media_server")) {
			/* the media_server doesn't use locked allocators because */
			/* it's so special */
			rtm_create_pool(&temp, size, NULL);
			_rtm_pool = temp;
		}
	} else {
		while(_rtm_pool == NULL)
			snooze(3000);
	}
}




static void * rtm_internal(rtm_pool * pool, size_t size);

static void rtm_validate_pool(rtm_pool * pool)
{
	area_info info;
	status_t err;
	rtm_block * bl, * en, * pr;
	rtm_free_list * fl, * fp;
	int total_free = 0;
	int total_blocks = 0;
	int free_size = 0;

	(void)valid_pool_ptr(pool);
	/* validate the pool */
	assert(pool != NULL,("NULL"));
	assert(pool->lock_count < 0,("%d\n", pool->lock_count));
	assert((err = get_area_info(pool->area, &info)) >= B_OK,("0x%x\n", err));
	assert(info.address == (void *)pool,("0x%x 0x%x\n", info.address, pool));
	assert(info.size == pool->total_size+sizeof(rtm_pool),("0x%x 0x%x", info.size, pool->total_size+sizeof(rtm_pool)));
	assert(pool->free_list.prev == NULL,("0x%x\n", pool->free_list.prev));
	assert(pool->free_blocks < pool->total_blocks || (pool->free_blocks == 1 && pool->total_blocks == 1), \
		("0x%x 0x%x\n", pool->free_blocks, pool->total_blocks));
	assert(pool->free_size <= pool->total_size-sizeof(rtm_block)*pool->free_blocks, \
		("0x%x 0x%x\n", pool->free_size, pool->total_size-sizeof(rtm_block)*pool->free_blocks));
	assert(pool->min_free_size <= pool->free_size,("0x%x, 0x%x\n", pool->min_free_size, pool->free_size));

	/* set up heap limits */
	bl = (rtm_block *)&pool[1];
	en = (rtm_block *)((char *)bl + pool->total_size);
	/* validate the free list */
	fl = &pool->free_list;
	fp = NULL;
	while (fl->next != NULL) {
		total_free++;
		if (total_free > pool->free_blocks+10) {
			break;	/* avoid infinite loops */
		}
		assert((char*)fl->next >= (char *)bl, ("0x%x 0x%x\n", fl->next, bl));
		assert((char*)fl->next < (char *)en, ("0x%x 0x%x\n", fl->next, en));
		assert(fl->next->prev == fl,("%d: 0x%x 0x%x 0x%x\n", total_free, fl, fl->next, fl->next->prev));
		assert(fl->prev == fp,("0x%x 0x%x 0x%x\n", fl, fl->prev, fp));
		pr = &((rtm_block *)fl->next)[-1];
		assert(pr->log_size == FREE_BLOCK,("0x%x 0x%x\n", pr, pr->log_size));
		free_size += pr->phys_size;
		fp = fl;
		fl = fl->next;
	}
	assert(total_free == pool->free_blocks,("%d %d\n", total_free, pool->free_blocks));
	assert(free_size == pool->free_size,("0x%x 0x%x\n", free_size, pool->free_size));

	/* validate the linear block count */
	assert(en > bl,("0x%x 0x%x\n", en, bl));
	pr = NULL;
	while (bl < en) {
		total_blocks++;
		assert(bl->pool == pool, ("0x%x 0x%x 0x%x\n", bl, bl->pool, pool));
		assert(bl >= (rtm_block*)&pool[1],("0x%x 0x%x 0x%x\n", bl, pool, &pool[1]));
		assert((char*)&bl[1]+bl->phys_size <= (char*)&pool[1]+pool->total_size,
			("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", bl, bl->phys_size, (char*)&bl[1]+bl->phys_size,
			pool, pool->total_size, (char*)&pool[1]+pool->total_size));
		assert(bl->phys_size >= sizeof(rtm_free_list),("0x%x 0x%x\n", bl, bl->phys_size));
		assert(!(bl->phys_size&(PADSIZE-1)), ("0x%x 0x%x\n", bl, bl->phys_size));
		assert(bl->log_size <= bl->phys_size, ("0x%x 0x%x 0x%x 0x%x\n", bl, bl->log_size, bl->phys_size));
		assert((bl->log_size == FREE_BLOCK) ||
			(bl->log_size >= (int32)((double)bl->phys_size*(double)REALLOC_SPLIT_PCT/100.0)) ||
			(bl->log_size < 0x10),
			("0x%x 0x%x 0x%x\n", bl, bl->log_size, bl->phys_size));
		assert(bl->log_size >= FREE_BLOCK,("0x%x 0x%x\n", bl, bl->log_size));
		assert(bl->prev == pr, ("0x%x 0x%x\n", bl, bl->prev, pr));
		pr = bl;
		bl = (rtm_block *)((char *)&bl[1]+bl->phys_size);
	}
	assert(bl == en,("0x%x 0x%x\n", bl, en));
	assert(total_blocks == pool->total_blocks,("%d %d\n", total_blocks, pool->total_blocks));
}


status_t rtm_create_pool(rtm_pool ** out_pool, size_t total_size, const char * name)
{
	return rtm_create_pool_etc(out_pool, total_size, name, 0);
}

status_t rtm_create_pool_etc(rtm_pool ** out_pool, size_t total_size, const char * name, uint32 flags, ...)
{
	rtm_pool p;
	rtm_block b;
	sem_id sem = -1;
	area_id area = -1;
	void * start = NULL;

	bool really_lock;
static bool checked = false;
static bool lock = true;

	if (out_pool == NULL)
	{
		name = "_default_rtm_pool_";
		__init_rtm_pool();              /* make sure it's created */
		out_pool = &_rtm_pool;
		if (_rtm_pool != NULL)
			return EALREADY;
	}
	if (name == NULL) {
		name = "_rtm_pool_NULL_";
	}

	if (!checked) {
		const char * destr;
		//	We know that these flags will always live last,
		//	because we put them there in media_server/MDefaultManager.cpp
		int fd = open(SETTING_FILE, O_RDONLY);
		uint32 term = 0;
		uint32 flags = 0;
		lseek(fd, -8, 2);
		if ((read(fd, &term, 4) < 4) || (term != 'mflg')) {
			goto defaults;
		}
		if (read(fd, &flags, 4) < 4) {
			system_info info;
defaults:
			get_system_info(&info);
			if (info.max_pages*B_PAGE_SIZE >= 63*1024*1024L) {
				lock = true;
			}
			else {
				lock = false;
			}
		}
		else {
			lock = (flags & B_MEDIA_REALTIME_ALLOCATOR);
		}
		close(fd);
		destr = getenv("MALLOC_DEBUG");
		if (destr) {
			is_debugging = atoi(destr);
			if (is_debugging < 1) is_debugging = 1;
		}
		checked = true;
	}

	really_lock = lock;
	if (flags & B_RTM_FORCE_LOCKED) {
		really_lock = true;
	}
	else if (flags & B_RTM_FORCE_UNLOCKED) {
		really_lock = false;
	}
	sem = create_sem(0, name);
	if (sem < 0) {
		goto bail;
	}
	total_size = (total_size+sizeof(rtm_pool)+sizeof(rtm_block)+
		(B_PAGE_SIZE-1)) & ~(B_PAGE_SIZE-1);
	PRINT(("create_area name=%s\n", name));
	area = create_area(name, &start, B_ANY_ADDRESS, total_size,
		really_lock ? B_FULL_LOCK : 0, B_READ_AREA | B_WRITE_AREA);
	if (is_debugging > 1) {
		fprintf(stderr, "rtm_create_area_etc(%ld, %s) returns %ld at 0x%lx\n", total_size, name, area, start);
	}
	if (area < 0 || !start) goto bail;
	if (is_debugging) {
		memset(start, FRESH_PAT, total_size);
	}

	p.next = 0;
	p.head = 0;
	p.lock_count = 0;
	p.lock_sem = sem;
	p.area = area;
	p.total_size = total_size-sizeof(rtm_pool);
	p.free_size = p.total_size-sizeof(rtm_block);
	p.min_free_size = p.free_size;
	p.total_blocks = 1;
	p.free_blocks = 1;
	*out_pool = (rtm_pool *)start;
	/*	User data (and free list data) live after the block headers.	*/
	p.free_list.next = (rtm_free_list *)&((rtm_block *)&(*out_pool)[1])[1];
	p.free_list.prev = NULL;
	p.free_list.next->next = NULL;
	p.free_list.next->prev = &(**out_pool).free_list;
	**out_pool = p;
	b.pool = *out_pool;
	b.prev = NULL;
	b.phys_size = p.free_size;	/* size of user block */
	b.log_size = FREE_BLOCK;
	*((rtm_block *)&(*out_pool)[1]) = b;
	return B_OK;

bail:
	if (sem >= 0) delete_sem(sem);
	if (area >= 0) delete_area(area);
	return (sem < 0) ? sem : (area < 0) ? area : B_ERROR;
}

status_t rtm_delete_pool(rtm_pool * pool)
{
	if (!pool) return B_BAD_VALUE;
	
	if (is_debugging > 1) {
		fprintf(stderr, "rtm_delete_pool(0x%lx) called\n", pool);
		(void)valid_pool_ptr(pool);
	}
	while (pool) {
		status_t err;
		rtm_pool *next_pool = pool->next;
		
		assert(pool->area > 0,("0x%x", pool->area));
		if (getenv("MEDIA_LOCKED_MEMORY_DEBUG") != 0) {
			rtm_dump_block(pool, NULL);
		}
	
		err = delete_area(pool->area);
		if (err)
			return err;
			
		pool = next_pool;
	}
	
	return B_OK;
}

void * rtm_alloc(rtm_pool * pool, size_t size)
{
	const char *ptr;
	int max_pools;
	int pool_size;
	rtm_pool *try_pool;
	rtm_pool *new_pool;
	int num_pools = 1;
	rtm_pool * unlock = NULL;
	void * ret;
	status_t err;
	char pool_name[32];
	area_info ainfo;
	char * name_end;

	if (pool == NULL)
	{
		__init_rtm_pool();            /* make sure it's initialized */
		pool = _rtm_pool;
		if (pool == NULL)
			return NULL;
	}
	try_pool = pool;

	//	carefully lock next pool before unlocking previous
	if (atomic_add(&try_pool->lock_count, -1) < 0) {
		acquire_sem(try_pool->lock_sem);
	}
	unlock = try_pool;
	while (true) {
		void *ret = rtm_internal(try_pool, size);
		if (ret) {
			if (atomic_add(&unlock->lock_count, 1) < -1) {
				release_sem(unlock->lock_sem);
			}
			return ret;
		}

		if (!try_pool->next)
			break;
			
		try_pool = try_pool->next;
		if (atomic_add(&try_pool->lock_count, -1) < 0) {
			acquire_sem(try_pool->lock_sem);
		}
		if (atomic_add(&unlock->lock_count, 1) < -1) {
			release_sem(unlock->lock_sem);
		}
		unlock = try_pool;
		num_pools++;
	}
	
	/* Couldn't allocate from existing pools */
//	PRINT(("rtm_alloc couldn't find memory in allocated pools\n"));
	ptr = getenv("MEDIA_MAX_ALLOCATOR_POOLS");
	max_pools = ptr ? atoi(ptr) : 8;
	if (max_pools > 20)
		max_pools = 20;

	if (num_pools >= max_pools) {
//		PRINT(("max_pools allocated... fail\n"));
		if (atomic_add(&unlock->lock_count, 1) < -1) {
			release_sem(unlock->lock_sem);
		}
		return 0;
	}
	
	pool_size = try_pool->total_size-sizeof(rtm_block);
	if (pool_size < 128000) pool_size = 128000;
	/* if (pool_size > 3*1024*1024) pool_size = 3*1024*1024; /* don't enforce limit when developer specifies size */
	PRINT(("allocate new pool size %d\n", pool_size));
	if (size > pool_size-sizeof(rtm_block)) {
		if (getenv("MEDIA_LOCKED_MEMORY_DEBUG") != 0) {
			fprintf(stderr, "Tried to allocate %d bytes from pool size %d\n", size, pool_size);
		}
		if (atomic_add(&unlock->lock_count, 1) < -1) {
			release_sem(unlock->lock_sem);
		}
		return 0;
	}

	//	create new name by lopping of digits from end and re-printing
	if (get_area_info(try_pool->area, &ainfo) != B_OK)
		ainfo.name[0] = 0;
	strncpy(pool_name, ainfo.name, 29);
	pool_name[29] = 0;
	name_end = &pool_name[strlen(pool_name)];
	while ((name_end > pool_name) && (name_end[-1] >= '0') && (name_end[-1] <= '9')) {
		name_end--;
	}
	sprintf(name_end, "%02d", num_pools%100);	//	guarantee only two digits in count

	err = rtm_create_pool(&try_pool->next, pool_size, pool_name);
	PRINT(("create new pool(%s -> %s): %s\n", ainfo.name, pool_name, strerror(err)));
	new_pool = try_pool->next;
	if (new_pool != NULL) {
		if (atomic_add(&new_pool->lock_count, -1) < 0) {
			acquire_sem(new_pool->lock_sem);
		}
		if(try_pool->head == NULL) {
			new_pool->head = try_pool;
		}
		else {
			new_pool->head = try_pool->head;
		}
	}
	if (atomic_add(&unlock->lock_count, 1) < -1) {
		release_sem(unlock->lock_sem);
	}
	unlock = new_pool;
	if (try_pool != NULL) {
		ret = rtm_internal(new_pool, size);
		if (atomic_add(&unlock->lock_count, 1) < -1) {
			release_sem(unlock->lock_sem);
		}
	}
	else {
		ret = NULL;
	}
	return ret;
}


//	MUST call rtm_internal with lock held
void * rtm_internal(rtm_pool * pool, size_t size)
{
	rtm_free_list * list = NULL;
	rtm_block * block = NULL;
	size_t padded_size = (((size > 0) ? size : 1) + PADSIZE-1) & ~(PADSIZE-1);

	assert(pool != NULL,("NULL\n"));

	if (!pool) {
		return NULL;
	}

	assert(pool->lock_count < 0,("%d\n", pool->lock_count));

	list = &pool->free_list;
	if (!list->next || (pool->free_size < size)) {
		return NULL;	/*	guaranteed nothing big enough	*/
	}
	while (list->next && (block = &((rtm_block *)list->next)[-1])->phys_size < size) {
		list = list->next;
	}
	if (!list->next) {
		return NULL;	/*	nothing big enough	*/
	}
	assert(block->log_size == FREE_BLOCK,("0x%x 0x%x\n", block, block->log_size));
	assert(block->pool == pool,("0x%x 0x%x 0x%x\n", block, block->pool, pool));
	assert(list->next->prev == list,("0x%x 0x%x 0x%x\n", list, list->next, list->next->prev));
	assert(block->phys_size >= padded_size,("0x%x 0x%x 0x%x\n", block, block->phys_size, padded_size));
	/*	split the block?	*/
	if (block->phys_size > padded_size+PADDING_OK) {
		size_t o_phys = block->phys_size;
		rtm_free_list * next_free = NULL;
		rtm_block * next = (rtm_block *)((char *)&block[1] + padded_size);
		rtm_block * after;
		/*	set up split-off block	*/
		next->phys_size = o_phys-padded_size-sizeof(rtm_block);
		next->log_size = FREE_BLOCK;
		next->prev = block;
		next->pool = pool;
		next_free = (rtm_free_list *)&next[1];
		next_free->next = list->next->next;
		if (next_free->next) {
			next_free->next->prev = next_free;
		}
		next_free->prev = list;
		list->next = next_free;
		/* set up block after that */
		after = (rtm_block *)((char *)&next[1]+next->phys_size);
		if (after < (rtm_block *)((char *)&pool[1] + pool->total_size)) {
			assert(after->log_size >= 0,("0x%x 0x%x\n", after, after->log_size));
			after->prev = next;
		}
		/*	set up current block	*/
		block->phys_size = padded_size;
		block->log_size = size;
		/*	update pool	*/
		pool->total_blocks++;
		pool->free_size -= padded_size+sizeof(rtm_block);
		if (pool->free_size < pool->min_free_size) {
			pool->min_free_size = pool->free_size;
//			fprintf(stderr, "min_free_size %x %x\n", pool->min_free_size, pool->free_size);
		}
//		fprintf(stderr, "TRACE: split new block at 0x%x\n", &next[1]);
	}
	else {	/*	just claim the block	*/
		/*	unlink from free list	*/
		if (list->next->next) {
			list->next->next->prev = list;
		}
		list->next = list->next->next;
		/*	set up current block	*/
		block->log_size = size;
		/*	update pool	*/
		pool->free_blocks--;
		pool->free_size -= block->phys_size;
		if (pool->free_size < pool->min_free_size) {
			pool->min_free_size = pool->free_size;
//			fprintf(stderr, "min_free_size %x %x\n", pool->min_free_size, pool->free_size);
		}
	}

//	fprintf(stderr, "TRACE: alloc(0x%x)\n", &block[1]);
	if (is_debugging > 1) rtm_validate_pool(pool);

	if (is_debugging) {
		memset(&block[1], ALLOC_PAT, block->phys_size);
	}
	return &block[1];
}

status_t rtm_free(void * data)
{
	rtm_pool * pool = NULL;
	rtm_block * block = NULL, *next = NULL, *prev = NULL;
	rtm_free_list * free_list = NULL, *next_free = NULL;

	if (data == NULL) {
		return B_BAD_VALUE;	/*	idiotic, but useful	*/
	}
	block = &((rtm_block *)data)[-1];
	pool = block->pool;

	assert(pool != NULL,("NULL\n"));
	assert(valid_pool_ptr(pool), ("data 0x%x, pool 0x%x\n", data, pool));
	assert(block->phys_size >= block->log_size,("0x%x 0x%x 0x%x\n", block, block->phys_size, block->log_size));
	assert(block->phys_size <= pool->total_size,("0x%x 0x%x 0x%x 0x%x\n", block, block->phys_size, pool, pool->total_size));
	assert(block->log_size != FREE_BLOCK,("FREE_BLOCK (twice freed block)"));

	if (atomic_add(&pool->lock_count, -1) < 0) {
		acquire_sem(pool->lock_sem);
	}

	if(is_debugging) {
		if (is_debugging > 1) rtm_validate_pool(pool);
		if(block->log_size == FREE_BLOCK) {
			fprintf(stderr, "%s:%d: data 0x%08x: Block freed twice\n", __FILE__, __LINE__, (int)data);
			if (is_debugging > 1) debugger("rtm_alloc() block freed twice!");
			if (atomic_add(&pool->lock_count, 1) < -1) {
				release_sem(pool->lock_sem);
			}
			return B_BAD_VALUE;
		}
	}

//	fprintf(stderr, "TRACE: free(0x%x)\n", data);
	if (is_debugging) {
		memset(&block[1], FREE_PAT, block->phys_size-sizeof(*block));
	}

	/*	we're freeing at least this amount	*/
	pool->free_size += block->phys_size;

	/*	is there a next block?	*/
	next = (rtm_block *)((char *)data+block->phys_size);
	if (next >= (rtm_block *)((char *)&pool[1]+pool->total_size)) {
		next = NULL;
	}
	/*	merge with next block?	*/
	if (next && next->log_size == FREE_BLOCK) {
		/*	fix up the free list	*/
		next_free = (rtm_free_list *)&next[1];
		assert(next_free->prev != NULL,("0x%x 0x%x\n", next_free, next_free->prev));
		/*	unlink next block from free list	*/
		if (next_free->next) {
			assert(next_free->next->prev == next_free,("0x%x 0x%x 0x%x\n", next_free, next_free->next, next_free->next->prev));
			next_free->next->prev = next_free->prev;
		}
		assert(next_free->prev->next == next_free,("0x%x 0x%x 0x%x\n", next_free, next_free->prev, next_free->prev->next));
		next_free->prev->next = next_free->next;
		/*	merge this block, freeing up a block header's worth of bytes	*/
		block->phys_size += next->phys_size + sizeof(rtm_block);
		pool->free_blocks--;	/* Because we unlinked it	*/
		pool->total_blocks--;
		pool->free_size += sizeof(rtm_block);
		/*	fix up next linear block's back pointer	*/
		next = (rtm_block *)((char *)&next[1] + next->phys_size);
		if (next < (rtm_block *)((char *)&pool[1]+pool->total_size)) {
			next->prev = block;
		}
		else {
			next = NULL;
		}

//		fprintf(stderr, "TRACE: merge next free block @ %x\n", &next[1]);
	}
	/*	merge with previous block?	*/
	prev = block->prev;
	if (prev && prev->log_size == FREE_BLOCK) {
		/*	we just stay in the free list	*/
		assert((rtm_block *)((char *)&prev[1]+prev->phys_size) == block,("0x%x 0x%x 0x%x\n", &prev[1], prev->phys_size, block));
		prev->phys_size += block->phys_size+sizeof(rtm_block);
		if (next) {
			next->prev = prev;
		}
		pool->total_blocks--;
		pool->free_size += sizeof(rtm_block);

//		fprintf(stderr, "TRACE: merge previous free block @ %x\n", &prev[1]);
	}
	else {
		/*	we need to link into the free list	*/
		free_list = (rtm_free_list *)&block[1];
		free_list->next = pool->free_list.next;
		free_list->prev = &pool->free_list;
		if (pool->free_list.next) {
			assert(pool->free_list.next->prev == &pool->free_list,("0x%x 0x%x 0x%x\n", &pool->free_list, pool->free_list.next, pool->free_list.next->prev));
			pool->free_list.next->prev = free_list;
		}
		pool->free_list.next = free_list;
		pool->free_blocks++;
	}
	//	always set this to better detect double frees
	block->log_size = FREE_BLOCK;

#if !NDEBUG
	rtm_validate_pool(pool);
#endif

	if (atomic_add(&pool->lock_count, 1) < -1) {
		release_sem(pool->lock_sem);
	}
	return B_OK;
}

// pool must be locked when calling _rtm_merge
// pass in next if the caller already did the work
// pool->min_free_size should be adjusted by the caller, in case
//	the merged block is split again before the pool is unlocked
// output block alloc state is the logical OR of each block's alloc state
//	BLOCK	NEXT	RESULT
//	free	free	free
//	free	alloc	alloc	(log_size = next->log_size)
//	alloc	free	alloc	(log_size = block->log_size)
//	alloc	alloc	alloc	(log_size = block->log_size + next->log_size)
// it's up to the caller to move any contents of next to be flush
//	with the end of block's data
// _rtm_merge takes care of getting the pool->free_* stuff synched

status_t _rtm_merge(rtm_pool * pool, rtm_block * block, rtm_block *next)
{
//! rtm_free could probably be convinced to use this function as well.
	rtm_free_list * free_list = NULL, *unlink_free = NULL;

	if(next == NULL) {
		next = (rtm_block *)((char *)&block[1]+block->phys_size);
		if (next >= (rtm_block *)((char *)&pool[1] + pool->total_size)) {
			return B_ERROR;
		}
	}

	// merge this block
	unlink_free = NULL;
	if(block->log_size == FREE_BLOCK) {
		if(next->log_size == FREE_BLOCK) {
			pool->free_size += sizeof(rtm_block);
			unlink_free = (rtm_free_list *)&next[1];
		}
		else {
			pool->free_size -= block->phys_size;
			// pool->min_free_size should be adjusted by the caller
			block->log_size = next->log_size;
			unlink_free = (rtm_free_list *)&block[1];
//			memmove((char*)&block[1], (char*)&next[1], next->log_size);
		}
	}
	else {
		if(next->log_size == FREE_BLOCK) {
			pool->free_size -= next->phys_size;
			// pool->min_free_size should be adjusted by the caller
			unlink_free = (rtm_free_list *)&next[1];
		}
		else {
			block->log_size += next->log_size;
//			memmove((char*)&block[1]+block->log_size, (char*)&next[1], next->log_size);
		}
	}
	pool->total_blocks--;
	block->phys_size += next->phys_size + sizeof(rtm_block);


	// fix up the free list
	if(unlink_free) {
		// unlink from free list
		assert(unlink_free->prev != NULL,("0x%x 0x%x\n", unlink_free, unlink_free->prev));
		if(unlink_free->next) {
			assert(unlink_free->next->prev == unlink_free,("0x%x 0x%x 0x%x\n", unlink_free, unlink_free->next, unlink_free->next->prev));
			unlink_free->next->prev = unlink_free->prev;
		}
		assert(unlink_free->prev->next == unlink_free,("0x%x 0x%x 0x%x\n", unlink_free, unlink_free->prev, unlink_free->prev->next));
		unlink_free->prev->next = unlink_free->next;
		pool->free_blocks--;
	}


	// fix up next linear block's back pointer
	next = (rtm_block *)((char *)&block[1] + block->phys_size);
	if (next < (rtm_block *)((char *)&pool[1]+pool->total_size)) {
		next->prev = block;
	}

	return B_OK;
}

// tries to exhibit realloc(3) behavior
status_t
rtm_realloc(void ** data, size_t new_size)
{
	rtm_pool * pool = NULL;
	rtm_block * block = NULL, *next = NULL, *prev = NULL;
	rtm_free_list * free_list = NULL, *next_free = NULL;
	size_t padded_size;
	size_t split;
	size_t o_log_size;
	char *n_data = NULL;

	// simple cases
	if(data == NULL) {
		return B_BAD_VALUE;
	}
	if(*data == NULL) {
		n_data = rtm_alloc(NULL, new_size);
		if(n_data) {
			*data = n_data;
			return B_OK;
		}
		return B_NO_MEMORY;
	}
	if(new_size == 0) {
		status_t s;
		s = rtm_free(*data);
		*data = NULL;
		return s;
	}

	block = &((rtm_block *)*data)[-1];
	pool = block->pool;

	if(block->log_size == new_size) {
		return B_OK;
	}

	assert(pool != NULL,("NULL\n"));
	assert(block->log_size != FREE_BLOCK,("realloc of FREE_BLOCK\n"));
	assert(block->phys_size >= block->log_size,("0x%x 0x%x 0x%x\n", block, block->phys_size, block->log_size));
	assert(block->phys_size <= pool->total_size,("0x%x 0x%x 0x%x 0x%x\n", block, block->phys_size, pool, pool->total_size));
	assert(block >= (rtm_block*)&pool[1],("0x%x 0x%x 0x%x\n", block, pool, &pool[1]));
	assert((char*)&block[1]+block->phys_size <= (char*)&pool[1]+pool->total_size,("0x%x 0x%x 0x%x 0x%x 0x%x 0x%x\n", block, block->phys_size, (char*)&block[1]+block->phys_size, pool, pool->total_size, (char*)&pool[1]+pool->total_size));

#if !NDEBUG
	// this is purely an internal consistency check,
	// since user has no control over contents of pool.
	// may go better in validate_pool, but this is the
	// only place we use pool->head...
	//
	// make sure block->pool is under pool->head
	// do this before the lock so we don't need
	// to deal with special cases
	if(is_debugging && pool->head != NULL) {
		rtm_pool *pp = pool->head;
		rtm_pool *unlock;

		if (atomic_add(&pp->lock_count, -1) < 0) {
			acquire_sem(pp->lock_sem);
		}
		unlock = pp;

		while (pp) {
			if(pool == pp) {
				break;
			}
			if(pp->next == NULL) {
				fprintf(stderr, "%s:%d: pool is not a proper sibling\n", __FILE__, __LINE__);
				if (atomic_add(&unlock->lock_count, 1) < -1) {
					release_sem(unlock->lock_sem);
				}
				return B_ERROR;
			}
			pp = pp->next;
			if (atomic_add(&pp->lock_count, -1) < 0) {
				acquire_sem(pp->lock_sem);
			}
			if (atomic_add(&unlock->lock_count, 1) < -1) {
				release_sem(unlock->lock_sem);
			}
			unlock = pp;
		}

		if (atomic_add(&unlock->lock_count, 1) < -1) {
			release_sem(unlock->lock_sem);
		}
	}
#endif

	if (atomic_add(&pool->lock_count, -1) < 0) {
		acquire_sem(pool->lock_sem);
	}

	// do this after the lock so freed blocks
	// don't slip through the cracks
	if(is_debugging) {
		if(block->log_size == FREE_BLOCK) {
			fprintf(stderr, "%s:%d: data 0x%08x: Realloc'ing a free block\n", __FILE__, __LINE__, (int)*data);
			debugger("rtm_realloc: realloc free block\n");
		}
	}

	o_log_size = block->log_size;

	padded_size = (((new_size > 0) ? new_size : 1) + PADSIZE-1) & ~(PADSIZE-1);

	//
	// how many cases are there?  let me count the ways...
	//

	// the new size is larger than the physical size of the block
	if(new_size > block->phys_size) {
		size_t next_size;
		size_t prev_size;

		// is there a free block after this one?
		next = (rtm_block *)((char *)&block[1]+block->phys_size);
		if (next >= (rtm_block *)((char *)&pool[1] + pool->total_size) ||
			next->log_size != FREE_BLOCK)
		{
			next = NULL;
		}
		next_size = next ? next->phys_size + sizeof(rtm_block) : 0;

		// can we avoid a copy by growing into the next block?
		if(padded_size <= block->phys_size + next_size) {
			_rtm_merge(pool, block, next);
			assert(new_size <= block->phys_size, ("%d 0x%x %d\n", new_size, block, block->log_size));
			n_data = *data;
			// could be split below
		}
		else {
			// is there a free block before this one?
			prev = block->prev;
			if (prev < (rtm_block*)&pool[1] ||
				prev->log_size != FREE_BLOCK)
			{
				prev = NULL;
			}
			prev_size = prev ? prev->phys_size + sizeof(rtm_block) : 0;

			// see if the previous block helps
			if(padded_size <= block->phys_size + prev_size ||
				padded_size <= block->phys_size + prev_size + next_size)
			{
				_rtm_merge(pool, prev, block);
				if(prev->phys_size < padded_size) {
					_rtm_merge(pool, prev, next);
				}
				block = prev;
				assert(new_size <= block->phys_size, ("%d 0x%x %d\n", new_size, block, block->phys_size));
				n_data = (char*)&block[1];
				memmove(n_data, *data, o_log_size);
				// could be split below
			}
		
			// need to allocate a new block and do a copy
			else {
				if(atomic_add(&pool->lock_count, 1) < -1) {
					release_sem(pool->lock_sem);
				}

				n_data = rtm_alloc(pool->head?pool->head:pool, new_size);
				if(n_data != NULL) {
					memcpy(n_data, *data, o_log_size);
					rtm_free(*data);
					*data = n_data;
					return B_OK;
				}
				return B_NO_MEMORY;
			}
		}
		split = new_size + PADDING_OK;
	}

	// there is enough physical space to simply change log_size.
	// split it if it's too small, though
	else {
		split = (int32)((double)new_size*100.0/(double)REALLOC_SPLIT_PCT);
		n_data = *data;
	}

	block->log_size = new_size;

	// by now, block should contain the new data.  if the
	// size of the block is greater than the 'split' value set
	// above, split it flush with the new data
	if(block->phys_size > split) {
//! this code could be put in a function and shared with rtm_alloc,
//  if no one's concerned about fn call overhead
		size_t o_phys;
		rtm_free_list * next_free = NULL, *list;
		rtm_block * after;

		// update current block
		o_phys = block->phys_size;
		block->phys_size = padded_size;
		// already changed log_size above

		// set up split-off block
		next = (rtm_block *)((char *)&block[1] + block->phys_size);
		next->phys_size = o_phys-block->phys_size-sizeof(rtm_block);
		next->log_size = FREE_BLOCK;
		next->prev = block;
		next->pool = pool;

		// update free list
		list = &pool->free_list;
		next_free = (rtm_free_list *)&next[1];
		next_free->next = list->next;
		if (next_free->next) {
			next_free->next->prev = next_free;
		}
		next_free->prev = list;
		list->next = next_free;

		// update pool
		pool->free_blocks++;
		pool->free_size += next->phys_size;
		pool->total_blocks++;
		
		// set up block after next
		after = (rtm_block *)((char *)&next[1]+next->phys_size);
		if (after < (rtm_block *)((char *)&pool[1] + pool->total_size)) {
			// if we are splitting the block the caller gave us,
			// the new block may have to be merged with its
			// successor
			if(after->log_size == FREE_BLOCK) {
				_rtm_merge(pool, next, after);
				after = (rtm_block *)((char *)&next[1]+next->phys_size);
				if (after < (rtm_block *)((char *)&pool[1] + pool->total_size)) {
					after->prev = next;
				}
			}
			else {
				after->prev = next;
			}
		}

		if(is_debugging) {
			memset((char*)&next_free[1], FREE_PAT, next->phys_size-sizeof(rtm_free_list));
		}

	}

	if(pool->free_size < pool->min_free_size) {
		pool->min_free_size = pool->free_size;
	}

	if(is_debugging && o_log_size < block->phys_size) {
		memset(n_data+o_log_size, ALLOC_PAT, block->phys_size-o_log_size);
	}

#if !NDEBUG
	rtm_validate_pool(pool);
#endif

	if (atomic_add(&pool->lock_count, 1) < -1) {
		release_sem(pool->lock_sem);
	}

	*data = n_data;

	return B_OK;
}

#if !NDEBUG

status_t rtm_pool_size(rtm_pool * pool)
{
	size_t size;

	if(pool == NULL) {
		return B_BAD_VALUE;
	}

	if (atomic_add(&pool->lock_count, -1) < 0) {
		acquire_sem(pool->lock_sem);
	}

	size = pool->total_size;

	if (atomic_add(&pool->lock_count, 1) < -1) {
		release_sem(pool->lock_sem);
	}

	return size;
}

rtm_pool *rtm_pool_for(void * data)
{
	rtm_pool * pool = NULL;
	rtm_block * block = NULL;
	size_t size = 0;

	if (data == NULL) {
		return NULL;
	}
	block = &((rtm_block *)data)[-1];
	return block->pool;
}

void rtm_pool_info(rtm_pool *pool)
{
	rtm_pool *unlock = NULL;
	int num_pools = 0;
	area_info ainfo;

	if (pool == NULL) {
		return;
	}

	if (atomic_add(&pool->lock_count, -1) < 0) {
		acquire_sem(pool->lock_sem);
	}

	unlock = pool;
	while (true) {
		get_area_info(pool->area, &ainfo);
		fprintf(stderr, "%29s(%d) 0x%08x: area %d, size 0x%08x (%d), free 0x%08x (%d), next 0x%08x, head 0x%08x\n", ainfo.name, num_pools, (int)pool, (int)pool->area, (int)pool->total_size, (int)pool->total_blocks, (int)pool->free_size, (int)pool->free_blocks, (int)pool->next, (int)pool->head);
		if (!pool->next)
			break;
			
		pool = pool->next;
		if (atomic_add(&pool->lock_count, -1) < 0) {
			acquire_sem(pool->lock_sem);
		}
		if (atomic_add(&unlock->lock_count, 1) < -1) {
			release_sem(unlock->lock_sem);
		}
		unlock = pool;
		num_pools++;
	}

	if (atomic_add(&unlock->lock_count, 1) < -1) {
		release_sem(unlock->lock_sem);
	}
}

#endif // !NDEBUG

status_t rtm_phys_size_for(void * data)
{
	rtm_pool * pool = NULL;
	rtm_block * block = NULL;
	size_t size = 0;

	if (data == NULL) {
		return B_BAD_VALUE;
	}
	block = &((rtm_block *)data)[-1];
	pool = block->pool;

	assert(pool != NULL,("NULL\n"));
	assert(block->phys_size >= block->log_size,("0x%x 0x%x 0x%x\n", block, block->phys_size, block->log_size));
	assert(block->phys_size <= pool->total_size,("0x%x 0x%x 0x%x 0x%x\n", block, block->phys_size, pool, pool->total_size));

	if (atomic_add(&pool->lock_count, -1) < 0) {
		acquire_sem(pool->lock_sem);
	}

	size = block->phys_size;

	if (atomic_add(&pool->lock_count, 1) < -1) {
		release_sem(pool->lock_sem);
	}

	return size;
}

status_t rtm_size_for(void * data)
{
	rtm_pool * pool = NULL;
	rtm_block * block = NULL;
	size_t size = 0;

	if (data == NULL) {
		return B_BAD_VALUE;	/*	idiotic, but useful	*/
	}
	block = &((rtm_block *)data)[-1];
	pool = block->pool;

	assert(pool != NULL,("NULL\n"));
	assert(block->phys_size >= block->log_size,("0x%x 0x%x 0x%x\n", block, block->phys_size, block->log_size));
	assert(block->phys_size <= pool->total_size,("0x%x 0x%x 0x%x 0x%x\n", block, block->phys_size, pool, pool->total_size));

	if (atomic_add(&pool->lock_count, -1) < 0) {
		acquire_sem(pool->lock_sem);
	}

	size = block->log_size;

	if (atomic_add(&pool->lock_count, 1) < -1) {
		release_sem(pool->lock_sem);
	}

	return size;
}

void rtm_dump_block(rtm_pool * pool, void * data)
{
	assert(pool != NULL,("NULL"));

	if (atomic_add(&pool->lock_count, -1) < 0) {
		/*	some bozo may have died holding the lock	*/
		acquire_sem_etc(pool->lock_sem, 1, B_TIMEOUT, 1000000);
	}
	fprintf(stderr, "--------------------------------------------------------------\n");
	if (data) {
		rtm_block * block = &((rtm_block *)data)[-1];
		assert(block->pool == pool,("0x%x 0x%x 0x%x\n", block, block->pool, pool));
		fprintf(stderr, "pool: 0x%x through 0x%x\n", pool, pool->total_size+sizeof(rtm_pool));
		fprintf(stderr, "block: 0x%x phys 0x%x log 0x%x prev 0x%x pool 0x%x\n", block,
			block->phys_size, block->log_size, block->prev, block->pool);
		if (block->log_size < 0) {
			rtm_free_list * fl = (rtm_free_list *)&block[1];
			fprintf(stderr, "   free_next: 0x%x free_prev: 0x%x\n", fl->next, fl->prev);
		}
	}
	else {
		rtm_block * block = (rtm_block *)&pool[1];
		fprintf(stderr, "pool: 0x%x first block: 0x%x end: 0x%x\n", pool,
			&((rtm_block*)&pool[1])[1], (char *)&pool[1] + pool->total_size);
		fprintf(stderr, "first free block: 0x%x\n", pool->free_list.next);
		fprintf(stderr, "total size: %x  free size: %x  min free: %x  total blocks: %x  free blocks: %x\n",
			pool->total_size, pool->free_size, pool->min_free_size, pool->total_blocks, pool->free_blocks);
		while (block < (rtm_block *)((char *)&pool[1] + pool->total_size)) {
			fprintf(stderr, "block: 0x%x phys 0x%x log 0x%x prev 0x%x pool 0x%x\n", block,
				block->phys_size, block->log_size, block->prev, block->pool);
			if (block->pool != pool) {
				fprintf(stderr, "\n### Damaged header; cannot continue!\n");
				break;
			}
			if (block->log_size < 0) {
				rtm_free_list * fl = (rtm_free_list *)&block[1];
				fprintf(stderr, "   free_next: 0x%x free_prev: 0x%x\n", fl->next, fl->prev);
			}
			block = (rtm_block *)((char *)&block[1] + block->phys_size);
		}
	}
	if (atomic_add(&pool->lock_count, 1) < -1) {
		release_sem(pool->lock_sem);
	}
}


rtm_pool *
rtm_default_pool()
{
	__init_rtm_pool();    /* make sure it's initialized */
	return _rtm_pool;
}

rtm_pool *
rtm_get_pool(rtm_pool * pool, void * block)
{
	rtm_pool * unlock = NULL;
	if (block == NULL) return NULL;	//	quick way out
	if (pool == NULL) {
		__init_rtm_pool();    /* make sure it's initalized */
		pool = _rtm_pool;
	}
	//	To be really safe, we lock the next block before
	//	we unlock the previous block.
	if (pool != NULL) {
		if (atomic_add(&pool->lock_count, -1) < 0) {
			acquire_sem(pool->lock_sem);
		}
		unlock = pool;
	}
	while (pool != NULL) {
		if (((char *)block > (char*)&pool[1]) &&
				((char *)block < &((char*)&pool[1])[pool->total_size])) {
			if (atomic_add(&unlock->lock_count, 1) < -1) {
				release_sem(unlock->lock_sem);
			}
			return pool;
		}
		pool = pool->next;
		if (pool != NULL) {
			if (atomic_add(&pool->lock_count, -1) < 0) {
				acquire_sem(pool->lock_sem);
			}
		}
		if (unlock != NULL) {
			if (atomic_add(&unlock->lock_count, 1) < -1) {
				release_sem(unlock->lock_sem);
			}
		}
		unlock = pool;
	}
	return NULL;
}
