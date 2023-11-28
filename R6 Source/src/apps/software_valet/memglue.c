#include "MyDebug.h"

#if (DEBUG && MEMDEBUG)

#define _MALLOC_INTERNAL
#include <malloc.h>

#include <OS.h>
#define	DEBUG	1
#include <Debug.h>

#define		SLOT_NUM		512
#define		TRACE_DEPTH		6
#define		INCR			128
#define		FREE_QUOTA		512

static int info_sem;

#ifdef __cplusplus
extern "C" {
#endif
void	infolock()
{
	acquire_sem(info_sem);
}

void	infounlock()
{
	release_sem(info_sem);
}

#ifdef __cplusplus
}
#endif
//extern void	_gnu_free(void *ptr, malloc_state *ms, malloc_funcs *mf);
//extern void	*_gnu_realloc(void *ptr, size_t size, malloc_state *ms, malloc_funcs *mf);

typedef struct heap_slot {
	//struct heap_slot	*prev;
	struct heap_slot	*next;
	void				*p;
	int					size;
	int					key;
	thread_id			thid;
	int					trace[TRACE_DEPTH];
} heap_slot;
	
typedef struct heap_state {
	heap_slot			*avail;
	heap_slot			*freed;
	heap_slot			*freedtail;
	int					key;
	int					nblk;
	int					freecount;
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

    //_m_acquire_sem(ms);
    infolock();
	key = ++(((heap_state *)(ms->data))->key);
  //  _m_release_sem(ms);
  	infounlock();

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

    //_m_acquire_sem(ms);
    infolock();
    
	PRINT(("\n"));
	for(i=0; i<SLOT_NUM; i++) {
		s = a->slots[i];
		while (s) {
			if (s->key == color) {
				PRINT(("%.8x (s. %5x, th. %3d).", s->p, s->size, s->thid));
				PRINT((" sc:"));
				for(j=0; j<TRACE_DEPTH; j++)
					PRINT((" %.8x", s->trace[j]));
				PRINT(("\n"));
			}
			s = s->next;
		}
	}
    // _m_release_sem(ms);
    infounlock();
}

static void *
my_malloc (size_t size, malloc_state *ms, malloc_funcs *mf)
{
	//PRINT(("my malloc\n"));

    void		*ptr;
	int			i, h;
	heap_state	*a;
	heap_slot	*newslot;
	int			*s;

    //_m_acquire_sem(ms);
    infolock();

	mf->old_malloc_hook = mf->malloc_hook;
	mf->malloc_hook = NULL;
    ptr = (void *)_malloc(size, ms, mf);
    mf->malloc_hook = mf->old_malloc_hook;
    
	a = (heap_state *) ms->data;
	if (a == NULL) {
		a = (heap_state	 *) _malloc_internal(sizeof(heap_state), ms, mf);
		ms->data = a;
		a->avail = NULL;
		a->key = 0;
		a->nblk = 0;
		a->freecount = 0;
		a->freed = NULL;
		a->freedtail = NULL;
		for(i=0; i<SLOT_NUM; i++)
			a->slots[i] = NULL;
	}

	h = hash_func(ptr);
	if (a->avail == NULL) {
		PRINT(("allocating more heap slots\n"));
		a->avail = (heap_slot *) _malloc_internal(INCR * sizeof(heap_slot), ms, mf);
		for(i=0; i<INCR; i++) {
			//a->avail[i].prev = (i == 0 ? NULL : &a->avail[i-1]);
			a->avail[i].next = (i == INCR-1 ? NULL : &a->avail[i+1]);		
		}
	}
	newslot = a->avail;
	a->avail = newslot->next;
	//if (a->avail)
	//	a->avail->prev = NULL;
	newslot->p = ptr;
	newslot->size = size;
	newslot->key = a->key;
	s = get_r1();
	for(i=-2; i<TRACE_DEPTH; i++) {
		s = (int *) (s[0]);
		if (!s)
			break;
		if (i >= 0)
			newslot->trace[i] = s[2];
	}
	newslot->thid = find_thread(NULL);
	
	//if (a->slots[h])
	//	a->slots[h]->prev = avail;
	//avail->prev = NULL;
	
	newslot->next = a->slots[h];
	a->slots[h] = newslot;
	a->nblk++;
	
    //_m_release_sem(ms);
    infounlock();

	return ptr;
}

static void
my_free (void *ptr, malloc_state *ms, malloc_funcs *mf)
{
	//PRINT(("my free\n"));
	int				h;
	heap_slot		*prev;
	heap_slot		*s;
	heap_state		*a;
	
	infolock();
    //_m_acquire_sem(ms);

	a = (heap_state *) ms->data;
	if (a != NULL) {
		h = hash_func(ptr);
		prev = NULL;
		s = a->slots[h];
		while (s != NULL) {
			if (s->p == ptr)
				break;
			prev = s;
			s = s->next;
		}
		if (s != NULL) {
			// this is a valid block
			
			if (prev)
				prev->next = s->next;
			else
				a->slots[h] = s->next;
			// add to the tail of freed list
			if (a->freedtail) {
				a->freedtail->next = s;
			}
			s->next = NULL;
			a->freedtail = s;
			if (!a->freed)
				a->freed = a->freedtail;
			
			// if freed quota is too high
			// free a block on the freed list
			// and move it to avail (and actually free the block)	
			if (a->freecount == FREE_QUOTA && a->freed) {
				prev = a->freed;
				a->freed = prev->next;
				
				prev->next = a->avail;
				a->avail = prev;
				
				a->nblk--;
				
				mf->old_free_hook = mf->free_hook;
				mf->free_hook = NULL;
				_free(prev->p,ms,mf);
				mf->free_hook = mf->old_free_hook;
			}
			else {
				a->freecount++;
			}
			// PRINT(("freecount is %d\n",a->freecount));
		}
		else {
			if (ptr != 0) {
				PRINT(("\t\t\tfree() of a non allocated block %.8x!!!\n",ptr));

				// search on the freed list
				
				s = a->freed;
				while(s != NULL) {
					if (s->p == ptr) {
						PRINT(("%.8x (s. %5x, th. %3d).", s->p, s->size, s->thid));
						PRINT((" sc:"));
						for(int j=0; j<TRACE_DEPTH; j++)
							PRINT((" %.8x", s->trace[j]));
						PRINT(("\n"));
						debugger("attempted free of non-allocated block");
						break;
					}
					s = s->next;
				}
			}
		}
	}
	else {
		PRINT(("free() called before malloc() !!!\n"));
	}

	//_free(ptr, ms, mf);

	// _m_release_sem(ms);
	
	//mf->old_free_hook = mf->free_hook;
	//mf->free_hook = NULL;
	//_free(ptr,ms,mf);
	//mf->free_hook = mf->old_free_hook;
	infounlock();
}

static void *
my_realloc (void *ptr, size_t size, malloc_state *ms, malloc_funcs *mf)
{
	// PRINT(("my realloc\n"));

	void		*ret;
	heap_state	*a;
	heap_slot	*prev;
	heap_slot	*s;
	int			h;

//	_m_acquire_sem(ms);
	infolock();
	mf->old_realloc_hook = mf->realloc_hook;
	mf->realloc_hook = NULL;

	ret = _realloc(ptr, size, ms, mf);
	
	mf->realloc_hook = mf->old_realloc_hook;
	
	if (!ret)
		goto exit;

	a = (heap_state *) ms->data;
	if (a != NULL) {
		h = hash_func(ptr);
		prev = NULL;
		s = a->slots[h];
		while (s != NULL) {
			if (s->p == ptr)
				break;
			prev = s;
			s = s->next;
		}
		if (s != NULL) {
			// this is a valid block
			// remove from the list
			
			if (prev)
				prev->next = s->next;
			else
				a->slots[h] = s->next;
								
			h = hash_func(ret);
			//if (a->slots[h])
			//	a->slots[h]->prev = s;
			//s->prev = NULL;
			s->next = a->slots[h];
			a->slots[h] = s;
	
			s->p = ret;
			s->size = size;
		} else
			PRINT(("realloc() of a non allocated block!!!\n"));
	} else
		PRINT(("realloc() called before malloc() !!!\n"));

exit:
	//_m_release_sem(ms);
	infounlock();

	return ret;
}
/**
extern malloc_state	gen_ms;
extern malloc_funcs	gen_mf;
**/

static malloc_state	*gen_ms = 0;
static malloc_funcs	*gen_mf = 0;

#ifdef	__cplusplus
extern "C"
{
#endif

void *get_gen_ms()
{
	debugger("set value of gen_ms to r3");
	return (void *)0xf0f0f0f0;
}

void *get_gen_mf()
{
	debugger("set value of gen_mf to r3");
	return (void *)0xf0f0f0f0;
}


#ifdef	__cplusplus
}
#endif


void
install_my_hooks()
{
	gen_ms = (malloc_state *)get_gen_ms();
	gen_mf = (malloc_funcs *)get_gen_mf();
	
	info_sem = create_sem(1, "debug_malloc");
	
	gen_mf->old_malloc_hook = _malloc;
	gen_mf->old_free_hook = _free;
	gen_mf->old_realloc_hook = _realloc;
	
	gen_mf->malloc_hook = my_malloc;
	gen_mf->free_hook = my_free;
	gen_mf->realloc_hook = my_realloc;
}

int
start_watching()
{
	return change_color(gen_ms);
}

void
stop_watching(int color)
{
	dump_blocks(gen_ms, color);
}

#endif
