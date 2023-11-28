//*****************************************************************************
//
//	File:		Message.cpp
//	
//	Written by:	Peter Potrebic & Cyril Meurillon
//
//	Copyright 1994, Be Incorporated, All Rights Reserved.
//
//*****************************************************************************

#ifdef DEBUG_SHARED_HEAP
#define DEBUG 1
#endif

#define _MALLOC_INTERNAL
#include <malloc.h>	

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#include <stdlib.h>
#include <string.h>

#ifndef _OS_H
#include <OS.h>
#endif
#ifndef _MESSAGE_UTIL_H
#include <message_util.h>
#endif
#ifndef _LOCKER_H
#include <Locker.h>
#endif

#include <priv_syscalls.h>
#include <AppDefsPrivate.h>

#ifdef DEBUG_SHARED_HEAP

#define		SLOT_NUM		512
#define		TRACE_DEPTH		4
#define		INCR			128

extern "C" void	_m_acquire_sem(malloc_state *ms);
extern "C" void	_m_release_sem(malloc_state *ms);
extern "C" void	_gnu_free(void *ptr, malloc_state *ms, malloc_funcs *mf);
extern "C" void	*_gnu_realloc(void *ptr, size_t size, malloc_state *ms, malloc_funcs *mf);

typedef struct heap_slot {
	struct heap_slot	*prev;
	struct heap_slot	*next;
	void				*p;
	int					size;
	int					key;
	team_id				tmid;
	int					trace[TRACE_DEPTH];
} heap_slot;
	
typedef struct heap_state {
	heap_slot			*avail;
	int					key;
	int					nblk;
	heap_slot			*slots[SLOT_NUM];
} heap_state;

static int
hash_func(void *p)
{
	return ((unsigned long) p & (SLOT_NUM-1)) ^
			 (((unsigned long) p >> 8) & (SLOT_NUM-1)) ^
			 (((unsigned long) p >> 16) & (SLOT_NUM-1)) ^
			 (((unsigned long) p >> 24) & (SLOT_NUM-1));
}

__asm static int *
get_r1(void)
{
	mr		r3, r1
	blr
}

static int
change_color(malloc_state *ms)
{
	heap_state		*a;
	int				key;

	a = (heap_state *) ms->data;
	if (a == NULL)
		return -1;

    _m_acquire_sem(ms);
	key = ++(((heap_state *)(ms->data))->key);
    _m_release_sem(ms);

	return key;
}

static void
dump_blocks(malloc_state *ms, int color)
{
	heap_state		*a;
	heap_slot		*s;
	int				i, j;

	a = (heap_state *) ms->data;
	if (a == NULL)
		return;

    _m_acquire_sem(ms);
	SERIAL_PRINT(("\n"));
	for(i=0; i<SLOT_NUM; i++) {
		s = a->slots[i];
		while (s) {
			if (s->key == color) {
				SERIAL_PRINT(("%.8x (s. %5x, tm. %3d).", s->p, s->size, s->tmid));
				SERIAL_PRINT((" sc:"));
				for(j=0; j<TRACE_DEPTH; j++)
					SERIAL_PRINT((" %.8x", s->trace[j]));
				SERIAL_PRINT(("\n"));
			}
			s = s->next;
		}
	}
    _m_release_sem(ms);
}

static void *
my_malloc (size_t size, malloc_state *ms, malloc_funcs *mf)
{
    void		*ptr;
	int			i, h;
	heap_state	*a;
	heap_slot	*avail;
	int			*s;
	thread_info	tinfo;

    _m_acquire_sem(ms);

    ptr = (void *)_malloc_internal(size, ms, mf);

	a = (heap_state *) ms->data;
	if (a == NULL) {
		a = (heap_state	 *) _malloc_internal(sizeof(heap_state), ms, mf);
		ms->data = a;
		a->avail = NULL;
		a->key = 0;
		a->nblk = 0;
		for(i=0; i<SLOT_NUM; i++)
			a->slots[i] = NULL;
	}

	h = hash_func(ptr);
	if (a->avail == NULL) {
		a->avail = (heap_slot *) _malloc_internal(INCR * sizeof(heap_slot), ms, mf);
		for(i=0; i<INCR; i++) {
			a->avail[i].prev = (i == 0 ? NULL : &a->avail[i-1]);
			a->avail[i].next = (i == INCR-1 ? NULL : &a->avail[i+1]);		
		}
	}
	avail = a->avail;
	a->avail = avail->next;
	if (a->avail)
		a->avail->prev = NULL;
	avail->p = ptr;
	avail->size = size;
	avail->key = a->key;
	s = get_r1();
	for(i=-2; i<TRACE_DEPTH; i++) {
		s = (int *) (s[0]);
		if (!s)
			break;
		if (i >= 0)
			avail->trace[i] = s[2];
	}
	get_thread_info(find_thread(NULL), &tinfo);
	avail->tmid = tinfo.team;
	if (a->slots[h])
		a->slots[h]->prev = avail;
	avail->prev = NULL;
	avail->next = a->slots[h];
	a->slots[h] = avail;
	a->nblk++;
	
    _m_release_sem(ms);

	return ptr;
}

static void
my_free (void *ptr, malloc_state *ms, malloc_funcs *mf)
{
	int				h;
	heap_slot		*s;
	heap_state		*a;

    _m_acquire_sem(ms);

	a = (heap_state *) ms->data;
	if (a == NULL) {
		SERIAL_PRINT(("free() called before malloc() !!!\n"));
		while (1);
	}

	h = hash_func(ptr);
	s = a->slots[h];
	while (s != NULL) {
		if (s->p == ptr)
			break;
		s = s->next;
	}
	if (s == NULL) {
		SERIAL_PRINT(("(tid=%d) free(%8x) of a non allocated block!!!\n",
			find_thread(NULL), ptr));
		long *p = (long *) ptr;
		SERIAL_PRINT(("  %8x %8x %8x %8x\n  %8x %8x %8x %8x\n  %8x %8x %8x %8x\n",
			p[0], p[1], p[2], p[3], p[4], p[5], p[6], p[8], p[9], p[10], p[11]));
		while (1);
	}

	if (s->prev)
		s->prev->next = s->next;
	else
		a->slots[h] = s->next;
	if (s->next)
		s->next->prev = s->prev;
	
	s->prev = NULL;
	s->next = a->avail;
	if (a->avail)
		a->avail->prev = s;
	a->avail = s;
	a->nblk--;

    _gnu_free(ptr, ms, mf);
    _m_release_sem(ms);
}

static void *
my_realloc (void *ptr, size_t size, malloc_state *ms, malloc_funcs *mf)
{
	void		*ret;
	heap_state	*a;
	heap_slot	*s;
	int			h;

	_m_acquire_sem(ms);
	ret = _gnu_realloc(ptr, size, ms, mf);

	if (!ret)
		goto exit;

	a = (heap_state *) ms->data;
	if (a == NULL) {
		SERIAL_PRINT(("realloc() called before malloc() !!!\n"));
		while (1);
	}

	h = hash_func(ptr);
	s = a->slots[h];
	while (s != NULL) {
		if (s->p == ptr)
			break;
		s = s->next;
	}
	if (s == NULL) {
		SERIAL_PRINT(("realloc() of a non allocated block!!!\n"));
		while (1);
	}

	if (s->prev)
		s->prev->next = s->next;
	else
		a->slots[h] = s->next;
	if (s->next)
		s->next->prev = s->prev;
	h = hash_func(ret);
	if (a->slots[h])
		a->slots[h]->prev = s;
	s->prev = NULL;
	s->next = a->slots[h];
	a->slots[h] = s;
	
	s->p = ret;
	s->size = size;

exit:
	_m_release_sem(ms);

	return ret;
}

#endif

//---------------------------------------------------------------

extern "C" {

// WARNING: be careful when changing this structure. Any changes will
// require a complete rebuild of the system. All code using this shared
// heap needs to be kept in synch.

typedef struct	ShrInfoStr {	/* info structure at start of shared area */
  	long		ShinSize;		/* Current byte size of area */
	area_id		aid;			/* Shared area id */
  	char	    *ShinArea;		/* Start of shared area for allocation logic */
	void		*data0;			/* Not used */
	void		*data1;			/* for future expansion */
	void		*data5;		/* ui parameters */
	void		*data2;			/* for future expansion */
	void		*data3;			/* for future expansion */
	void		*data4;			/* for future expansion */
} ShrInfoStr, * ShrInfoPtr;

static ShrInfoPtr SharedInfo = NULL;	/* shared area info */

#define	SharedVirtualStart	(0x72000000)	 /* Absolute virtual address for shared area */
#define	SharedInitSize		(0x1000)	     /* Start with one page */
#define	ShrInfoSize			(sizeof(ShrInfoStr) + sizeof(malloc_state))

#if 0
void	*_get_shared_ui_info_()
{
	return SharedInfo->ui_info;
}

void	_set_shared_ui_info_(void *ui_info)
{
	SharedInfo->ui_info = ui_info;
}
#endif

static		char *sh_sbrk(long size);
static		void *sh_morecore(ptrdiff_t increment, malloc_state *ms);

malloc_state *sh_ms;
malloc_funcs  sh_mf = { 0, };

static area_id first_aid;

#ifdef DEBUG_SHARED_HEAP
#define DEBUG_SHARED_HEAP_ONLY(x) x
#else
#define DEBUG_SHARED_HEAP_ONLY(x) 
#endif

extern "C" int start_watching(void);
extern "C" void stop_watching(int);

extern "C" int start_watching(void)
{
#ifdef DEBUG_SHARED_HEAP
	return (change_color(sh_ms));
#else
	return -1;
#endif
}

extern "C" void stop_watching(int DEBUG_SHARED_HEAP_ONLY(color))
{
#ifdef DEBUG_SHARED_HEAP
	dump_blocks(sh_ms, color);
#endif
}

int _init_shared_heap_()
{
	void *tmp_area_ptr;
	area_id	shaid;
	
	tmp_area_ptr = SharedVirtualStart;
	if (area_for(tmp_area_ptr) < 0) {
		shaid = create_area("shared heap", 
							&tmp_area_ptr,
							B_EXACT_ADDRESS,
							SharedInitSize,
							B_NO_LOCK,
							B_READ_AREA | B_WRITE_AREA);

		// if create_area returned an error than we have entered into
		// a race condition with another client and are hoping that
		// they have actually succeeded in doing an area_create - if
		// so then our second test of area_which will return NO_ERR
		// and we can safely return - if not then we are in trouble
		if (shaid < 0) {
 			if (area_for(tmp_area_ptr) < 0) {
 				debugger("Script virtual memory not allocated at absolute virtual address\n");
 			}
		} else {
			SharedInfo = (ShrInfoPtr)SharedVirtualStart;
			SharedInfo->ShinSize = SharedInitSize;
			SharedInfo->ShinArea = (char *)SharedInfo + SharedInitSize;
			SharedInfo->aid = shaid;

			sh_ms = (malloc_state *)((char *)SharedVirtualStart +
									 sizeof(struct ShrInfoStr));

			memset(sh_ms, 0, sizeof(malloc_state));

			sh_ms->malloc_sem  = create_sem(0, "sh_malloc");
			sh_ms->malloc_lock = 0;
		}
	}

	SharedInfo = (ShrInfoPtr)SharedVirtualStart;
	sh_ms      = (malloc_state *)((char *)SharedVirtualStart +
								  sizeof(ShrInfoStr));

	memset(&sh_mf, 0, sizeof(malloc_funcs));
	sh_mf.free_memory = _kfree_memory_;
	sh_mf.morecore = sh_morecore;
#ifdef DEBUG_SHARED_HEAP
	sh_mf.malloc_hook = my_malloc;
	sh_mf.free_hook = my_free;
	sh_mf.realloc_hook = my_realloc;
#endif

	return B_NO_ERROR;
}

/*------------------------------------------------------------*/

 
static char*	sh_sbrk(long size)
{
        char    *old_pointer;
 
        if (size > 50*1024*1024) {
			debugger("size is ridiculous !!\n");
		}

		if (size == 0)
			return SharedInfo->ShinArea;

		old_pointer = SharedInfo->ShinArea;
        SharedInfo->ShinSize += size;

        if(resize_area(SharedInfo->aid, SharedInfo->ShinSize) < B_NO_ERROR) {
        	debugger("Could not extend shared memory area\n");
        }
		SharedInfo->ShinArea += size;

        return(old_pointer);
}
 
 
static void *sh_morecore(ptrdiff_t increment, malloc_state *)
{
	void *result = (void *) sh_sbrk(increment);

	if (result == (void *) -1)
		return NULL;

	return result;
}


 
void  *sh_malloc(size_t nbytes)
{
	return _malloc(nbytes, sh_ms, &sh_mf);
}
 

void sh_free(void *ptr)
{
	_free(ptr, sh_ms, &sh_mf);
}
 
 
/*------------------------------------------------------------*/
 
void *sh_realloc(void *p, size_t size)
{
	return _realloc(p, size, sh_ms, &sh_mf);
}


static void dumpfreelist()
{
	printf("dumpfreelist: not implemented yet for gnu malloc\n");
}

static void sh_heap_check()
{
	printf("sh_heap_check not done for gnu malloc. see mcheck()\n");
}


}	// End of extern "C"
