/*
   these are the standard C library wrappers for the _malloc(), etc
   routines.
*/

#ifndef	_MALLOC_INTERNAL
#define	_MALLOC_INTERNAL
#include "malloc.h"
#endif
#include <OS.h>
#include <priv_syscalls.h>
#include <errno.h>
#include <fcntl.h>

#include <stdlib.h>

#include "malloc_priv.h"

extern bool	_single_threaded;

static malloc_state gen_ms = { -1, };
static malloc_funcs gen_mf;

#ifdef __MWERKS__
#define UNUSED(x)
#else
#define UNUSED(x) x
#endif

#include "LeakChecking.h"


#define MALLOC_COLLISION_STATS 0


#if (defined(__INTEL__) || defined(__arm__))  &&  _SUPPORTS_LEAK_CHECKING	/* FIXME: Is this a glibc dependency or something */
# define LEAK_CHECKING 1
#endif

#if LEAK_CHECKING

static void *traced_malloc(size_t size, malloc_state *ms, malloc_funcs *mf);
static void traced_free(void *ptr, malloc_state *ms, malloc_funcs *mf);
static void *traced_realloc(void *ptr, size_t size, malloc_state *ms, malloc_funcs *mf);
static void *traced_memalign(size_t alignment, size_t size, malloc_state *ms, malloc_funcs *mf);

static void log_malloc(char type, void *addr, int length, int alignment);
static char malloc_log[4096 * 3];
static int malloc_log_index = 0;
static int malloc_log_fd;

#if MALLOC_COLLISION_STATS
static int malloc_collision_fd = -1;
#endif

void *
unchecked_malloc(size_t size)
{
	if (gen_mf.malloc_hook)
		return gen_mf.malloc_hook(size, &gen_ms, &gen_mf);
	
	return _malloc(size, &gen_ms, &gen_mf);
}

void
unchecked_free(void *ptr)
{
	if (gen_mf.free_hook)
		gen_mf.free_hook(ptr, &gen_ms, &gen_mf);
	else
		_free(ptr, &gen_ms, &gen_mf);
}

void *
unchecked_realloc(void *ptr, size_t size)
{
	if (gen_mf.realloc_hook)
		return gen_mf.realloc_hook(ptr, size, &gen_ms, &gen_mf);

	return _realloc(ptr, size, &gen_ms, &gen_mf);
}
#endif

#if LEAK_CHECKING
void *
malloc(size_t size)
{
	void *ptr;
	if (gen_mf.malloc_hook)
		ptr = gen_mf.malloc_hook(size, &gen_ms, &gen_mf);
	else
		ptr = _malloc(size, &gen_ms, &gen_mf);

	if (ptr && MallocLeakChecking())
		record_malloc(ptr, size);
	
	return ptr;
}

void *
calloc(size_t nmemb, size_t size)
{
	void *ptr;
	
	if (gen_mf.malloc_hook) {
		ptr = gen_mf.malloc_hook(size * nmemb, &gen_ms, &gen_mf);
	} else {
		ptr = _malloc(size * nmemb, &gen_ms, &gen_mf);
	}
	
	if (ptr != NULL)
		memset(ptr, 0, nmemb * size);

	if (ptr && MallocLeakChecking())
		record_malloc(ptr, size);

	return ptr;
}

void *
realloc(void *ptr, size_t size)
{
	void *result;
	if (gen_mf.realloc_hook)
		result = gen_mf.realloc_hook(ptr, size, &gen_ms, &gen_mf);
	else
		result = _realloc(ptr, size, &gen_ms, &gen_mf);

	if (result && MallocLeakChecking()) 
		record_realloc(result, ptr, size);

	return result;
}

//#define DUMP_HEAP_FREE

#ifdef DUMP_HEAP_FREE
static int32 dumpHeapCount = 0;
#endif

void
free(void *ptr)
{
	if (ptr && MallocLeakChecking())
		record_free(ptr);

	if (gen_mf.free_hook)
		gen_mf.free_hook(ptr, &gen_ms, &gen_mf);
	else
		_free(ptr, &gen_ms, &gen_mf);

#ifdef DUMP_HEAP_FREE
	if (MallocLeakChecking() && ((++dumpHeapCount) % 1000) == 0)
		printf("heap: bytes used %ld, bytes free %ld, chunks used %ld, chunks free %ld\n",
			gen_ms._bytes_used, gen_ms._bytes_free, gen_ms._chunks_used, gen_ms._chunks_free);
#endif
}

#else

void *
malloc(size_t size)
{
	if (gen_mf.malloc_hook)
		return gen_mf.malloc_hook(size, &gen_ms, &gen_mf);
	
	return _malloc(size, &gen_ms, &gen_mf);
}

void *
calloc(size_t nmemb, size_t size)
{
	void *ptr;
	
	if (gen_mf.malloc_hook) {
		ptr = gen_mf.malloc_hook(size * nmemb, &gen_ms, &gen_mf);
	} else {
		ptr = _malloc(size * nmemb, &gen_ms, &gen_mf);
	}
	
	if (ptr != NULL)
		memset(ptr, 0, nmemb * size);

	return ptr;
}

void *
realloc(void *ptr, size_t size)
{
	if (gen_mf.realloc_hook)
		return gen_mf.realloc_hook(ptr, size, &gen_ms, &gen_mf);

	return _realloc(ptr, size, &gen_ms, &gen_mf);
}


void
free(void *ptr)
{
	if (gen_mf.free_hook)
		gen_mf.free_hook(ptr, &gen_ms, &gen_mf);
	else
		_free(ptr, &gen_ms, &gen_mf);
}

#endif

void
cfree(void *ptr)
{
	if (gen_mf.free_hook)
		gen_mf.free_hook(ptr, &gen_ms, &gen_mf);
	else
		_free(ptr, &gen_ms, &gen_mf);
}

void *
memalign(size_t alignment, size_t size)
{
  if (gen_mf.memalign_hook)
    return gen_mf.memalign_hook (alignment, size, &gen_ms, &gen_mf);
  else
	return _memalign(alignment, size, &gen_ms, &gen_mf);
}

void *
valloc(size_t size)
{
	return _valloc(size, &gen_ms, &gen_mf);
}

struct mstats
mstats (void)
{
	return _mstats(&gen_ms, &gen_mf);
}

void *
malloc_find_object_address (void *ptr)
{
	return _malloc_find_object_address(ptr, &gen_ms, &gen_mf);
}


#if _SUPPORTS_MALLOC_DEBUG
int
mcheck (void (*func) (enum mcheck_status))
{
	return _mcheck(func, &gen_ms, &gen_mf);
}

extern enum mcheck_status _mprobe(void *, malloc_funcs *);

enum mcheck_status
mprobe (void *ptr)
{
	return _mprobe(ptr, &gen_mf);
}
#endif

static void *
gen_morecore(ptrdiff_t increment, malloc_state * UNUSED(junk))
{
  void *result = (void *) sbrk((int) increment);

  if (result == (void *) -1)
    return NULL;
  return result;
}

#if MALLOC_COLLISION_STATS
extern uint64 atomic_add64(uint64 *a, uint64 b);
static uint64 _malloc_collision_ops = 0;
static uint64 _malloc_collision_hits = 0;
static bigtime_t _malloc_collision_time = 0;
static bigtime_t _malloc_collision_max = 0;
#endif

void	_m_acquire_sem(malloc_state *ms)
{
    int	old;

	if (!_single_threaded) {
#if MALLOC_COLLISION_STATS
		atomic_add64(&_malloc_collision_ops, 1);
#endif
    	old = atomic_add (&ms->malloc_lock, 1);
    	if (old >= 1) {
#if MALLOC_COLLISION_STATS
			bigtime_t stall;
			bigtime_t start = system_time();
#endif
			while (acquire_sem(ms->malloc_sem) == B_INTERRUPTED)
				;
#if MALLOC_COLLISION_STATS
			_malloc_collision_hits++;
			stall = system_time() - start;
			if (stall > _malloc_collision_max) _malloc_collision_max = stall;
			_malloc_collision_time += stall;
#endif
    	}	
	}
}

void	_m_release_sem(malloc_state *ms)
{
    int	old;

	if (!_single_threaded) {
    	old = atomic_add (&ms->malloc_lock, -1);
    	if (old > 1) {
			release_sem(ms->malloc_sem);
    	}
	}
}

void
__lock_gen_malloc()
{
   _m_acquire_sem(&gen_ms);
}


void
__unlock_gen_malloc()
{
   _m_release_sem(&gen_ms);
}

static void
itoa(char *buf, int num)
{
	char digits[] = "0123456789abcdef";
	char obuff[11];
	int i = 9;
	unsigned long quotient, rem;

	obuff[10] = 0;
	while (i > 0  &&  num > 0) {
		quotient = num / 10;
		rem = num % 10;
		obuff[i--] = digits[rem];
		num = quotient;
	}
	strcpy(buf, &obuff[i+1]);
}

void
_init_gen_malloc(void)
{
	char *md;
#if LEAK_CHECKING
	char *mt;
#endif

	memset(&gen_ms, 0, sizeof(gen_ms));

	gen_ms.malloc_sem  = create_sem(0, "gen_malloc");
	if (gen_ms.malloc_sem < B_NO_ERROR)
		abort();

	gen_ms.malloc_lock = 0;

	memset(&gen_mf, 0, sizeof(gen_mf));
	gen_mf.morecore    = gen_morecore;
	gen_mf.free_memory = &_kfree_memory_;

#if _SUPPORTS_MALLOC_DEBUG
	md = getenv("MALLOC_DEBUG");
	if (md) {
		mcheck(0);
	}
#endif
#if LEAK_CHECKING
	mt = getenv("MALLOC_TRACE");
	if (mt  && atoi(mt) != 0) {
		char fname[30];

		gen_mf.malloc_hook = traced_malloc;
		gen_mf.free_hook = traced_free;
		gen_mf.realloc_hook = traced_realloc;
		gen_mf.memalign_hook = traced_memalign;
		
		strcpy(fname, "malloc_log.");
		itoa(fname + 11, getpid());
		malloc_log_fd = open(fname, O_RDWR | O_CREAT, 0644);
	}
# if MALLOC_COLLISION_STATS
	{
		char *mc = getenv("MALLOC_COLLISIONS");
		if (mc && (atoi(mc) != 0)) {
			char fname[64];
			strcpy(fname, "/tmp/malloc_collisions.");
			itoa(fname+23, getpid());
			malloc_collision_fd = open(fname, O_RDWR | O_CREAT, 0644);
		}	
	}
# endif
#endif	
}

/* POSIX fork support... */
void
__fork_gen_malloc(void)
{
	gen_ms.malloc_sem	= create_sem(0, "gen_malloc");
	gen_ms.malloc_lock	= 0;

#if LEAK_CHECKING
	if (malloc_log_fd > 0) {
		char fname[30];
		close(malloc_log_fd);
		strcpy(fname, "malloc_log.");
		itoa(fname + 11, getpid());
		malloc_log_fd = open(fname, O_RDWR | O_CREAT, 0644);
	}
#endif
}


void
_cleanup_gen_malloc(void)
{
	delete_sem(gen_ms.malloc_sem);
	gen_ms.malloc_sem         = -1;
	gen_ms.malloc_lock        = 0;
	gen_ms.malloc_initialized = 0;

#if LEAK_CHECKING
	if (malloc_log_fd > 0) {
		if (malloc_log_index > 0)
			write(malloc_log_fd, malloc_log, malloc_log_index);
		close(malloc_log_fd);
	}
# if MALLOC_COLLISION_STATS
	if (malloc_collision_fd > 0) {
		write(malloc_collision_fd, &_malloc_collision_ops, sizeof(uint64));
		write(malloc_collision_fd, &_malloc_collision_hits, sizeof(uint64));
		write(malloc_collision_fd, &_malloc_collision_time, sizeof(bigtime_t));
		write(malloc_collision_fd, &_malloc_collision_max, sizeof(bigtime_t));
		close(malloc_collision_fd);
	}
# endif
#endif
}


#if LEAK_CHECKING
static void
log_realloc(void *old, void *new, int length)
{
	char buffer[20];

	buffer[0] = 'r';
	*(int *)(&buffer[1]) = (int) old;
	*(int *)(&buffer[5]) = (int) new;
	*(int *)(&buffer[9]) = (int) length;
	memcpy(malloc_log + malloc_log_index, buffer, 13);
	malloc_log_index += 13;
	if (malloc_log_index >= ((4096 * 3) - 20)) {
		write(malloc_log_fd, malloc_log, malloc_log_index);
		malloc_log_index = 0;
	}
}

static void
log_malloc(char type, void *addr, int length, int alignment)
{
	int l;
	char buffer[20];
	int i = 0;
	
//_sPrintf("log_malloc: %c %x %d %d\n", type, addr, length, alignment);
	buffer[i++] = type;

	switch (type) {
	case 'f':
		*(int *)(&buffer[i]) = (int) addr;
		l = 5;
		break;
	case 'm':
		*(int *)(&buffer[i]) = (int) addr;
		*(int *)(&buffer[i+4]) = length;
		l = 9;
		break;
	case 'a':
		*(int *)(&buffer[i]) = (int) addr;
		*(int *)(&buffer[i+4]) = length;
		*(int *)(&buffer[i+8]) = alignment;
		l = 13;
		break;
	}

	memcpy(malloc_log + malloc_log_index, buffer, l);
	malloc_log_index += l;
	if (malloc_log_index >= ((4096 * 3) - 20)) {
		write(malloc_log_fd, malloc_log, malloc_log_index);
		malloc_log_index = 0;
	}
}

static void *
traced_malloc(size_t size, malloc_state *ms, malloc_funcs *mf)
{
	void *ptr;
	_m_acquire_sem(ms);
	ptr = _malloc_internal(size, ms, mf);
	log_malloc('m', ptr, size, 0);
	_m_release_sem(ms);

	return ptr;
}

static void
traced_free(void *ptr, malloc_state *ms, malloc_funcs *mf)
{
	_m_acquire_sem(ms);
	_free_internal(ptr, ms, mf);
	log_malloc('f', ptr, 0, 0);
	_m_release_sem(ms);
}

static void *
traced_realloc(void *ptr, size_t size, malloc_state *ms, malloc_funcs *mf)
{
	void * ret;
	char type;

	_m_acquire_sem(ms);
	ret = _realloc_internal(ptr, size, ms, mf);
	log_realloc(ptr, ret, size);
	_m_release_sem(ms);

	return ret;
}

static void *
traced_memalign(size_t alignment, size_t size, malloc_state *ms, malloc_funcs *mf)
{
  void *ptr;
  
  _m_acquire_sem(ms);
  ptr = _memalign_internal(alignment, size, ms, mf);
  log_malloc('a', ptr, size, alignment);
  _m_release_sem(ms);

  return ptr;
}
#endif
