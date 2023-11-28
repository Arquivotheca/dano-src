/*
   This file contains the structures that malloc uses to manage
   memory.  It is not intended for public consumption.

*/

/* SVID2/XPG mallinfo structure */

struct mallinfo {
  int arena;    /* total space allocated from system */
  int ordblks;  /* number of non-inuse chunks */
  int smblks;   /* unused -- always zero */
  int hblks;    /* number of mmapped regions */
  int hblkhd;   /* total space in mmapped regions */
  int usmblks;  /* unused -- always zero */
  int fsmblks;  /* unused -- always zero */
  int uordblks; /* total allocated space */
  int fordblks; /* total non-inuse space */
  int keepcost; /* top-most, releasable (via malloc_trim) space */
};	

/* SVID2/XPG mallopt options */

#define M_MXFAST  1    /* UNUSED in this malloc */
#define M_NLBLKS  2    /* UNUSED in this malloc */
#define M_GRAIN   3    /* UNUSED in this malloc */
#define M_KEEP    4    /* UNUSED in this malloc */

/* mallopt options that actually do something */

#define M_TRIM_THRESHOLD    -1
#define M_TOP_PAD           -2
#define M_MMAP_THRESHOLD    -3
#define M_MMAP_MAX          -4

#ifndef INTERNAL_SIZE_T
# define INTERNAL_SIZE_T size_t
#endif

struct malloc_chunk
{
  INTERNAL_SIZE_T prev_size; /* Size of previous chunk (if free). */
  INTERNAL_SIZE_T size;      /* Size in bytes, including overhead. */
  struct malloc_chunk* fd;   /* double links -- used only if free. */
  struct malloc_chunk* bk;
};

#define NAV             128   /* number of bins */

typedef struct malloc_chunk* mbinptr;

/*
   This is all the state that the malloc routines need to know about.
   A pointer to this structure is passed along with the normal arguments
   by the wrapper routines.
*/
typedef struct malloc_state {

    int  malloc_sem;  /* semaphore & lock to serialize access, must be first */
    long malloc_lock;

	/* Are you experienced?  */
	int malloc_initialized;

	/* bins */
	mbinptr av_[NAV * 2 + 2];
	
	/* variables holding tunable values */
	unsigned long trim_threshold;
	unsigned long top_pad;
	unsigned int n_mmaps_max;
	unsigned long mmap_threshold;

	/* first value returned from sbrk */
	char *sbrk_base;

	/* maximum memory obtained from system via sbrk */
	unsigned long max_sbrked_mem;

	/* maximum via either sbrk or mmap */
	unsigned long max_total_mem;

	/* internal working copy of mallinfo */
	struct mallinfo current_mallinfo;

	/* tracking mmaps */
	unsigned long n_mmaps;
	unsigned long max_n_mmaps;
	unsigned long mmapped_mem;
	unsigned long max_mmapped_mem;

	/* private data used by the hooks */
	void *data;
} malloc_state;


/*
   This contains function pointers that are used by the underlying
   GNU malloc code.  They have to be separated from the malloc_state
   structure because someone that is linked statically would have the
   routines at a different address than someone who isn't linked
   statically and if they share the same malloc_state structure then
   problems would arise.  This is really only a problem for shared 
   malloc pools but nonetheless we must deal with it.
*/   
typedef struct malloc_funcs
{
	/* free_memory function or NULL */
	long	(*free_memory)(void *ptr, size_t size);

	/* How to really get more memory.  */
	void *(*morecore) (ptrdiff_t size, struct malloc_state *ms);

	/* Debugging hooks */
	void *(*malloc_hook)  (size_t size, malloc_state *ms, struct malloc_funcs *mf);
	void  (*free_hook)    (void *ptr,   malloc_state *ms, struct malloc_funcs *mf);
	void *(*realloc_hook) (void *ptr,   size_t size,  malloc_state *ms, struct malloc_funcs *mf);
	void *(*memalign_hook)(size_t size, size_t align, malloc_state *ms, struct malloc_funcs *mf);

	/* saved hook values, used by mcheck  */
	void *(*old_malloc_hook)  (size_t size, malloc_state *ms, struct malloc_funcs *mf);
	void  (*old_free_hook)    (void *ptr,   malloc_state *ms, struct malloc_funcs *mf);
	void *(*old_realloc_hook) (void *ptr,   size_t size,  malloc_state *ms, struct malloc_funcs *mf);
	void *(*old_memalign_hook)(size_t size, size_t align, malloc_state *ms, struct malloc_funcs *mf);

	/* a function called by mcheck() when something awful happens.  */
	void (*abortfunc) (enum mcheck_status);
	
	void (*malloc_initialize_hook) (void);
	void (*after_morecore_hook) (void);

}malloc_funcs;

/* Internal version of `free' used in `morecore' (malloc.c). Do Not Call. */
extern void  _free_internal   (void * __ptr,  malloc_state *ms, malloc_funcs *mf);
extern void *_malloc_internal (size_t __size, malloc_state *ms, malloc_funcs *mf);

/* these are the routines you write your wrappers for */
extern void *_malloc  (size_t size,  malloc_state *ms, malloc_funcs *mf);
extern void *_calloc  (size_t nmemb, size_t size,      malloc_state *ms, malloc_funcs *mf);
extern void *_realloc (void * ptr,   size_t size,      malloc_state *ms, malloc_funcs *mf);
extern void *_valloc  (size_t size,  malloc_state *ms, malloc_funcs *mf);
extern void *_memalign(size_t align, size_t size,      malloc_state *ms, malloc_funcs *mf);
extern void  _free    (void *ptr,    malloc_state *ms, malloc_funcs *mf);
extern void  _cfree   (void *ptr,    malloc_state *ms, malloc_funcs *mf);

extern int _mcheck (void (*func) (enum mcheck_status), malloc_state *ms,
					malloc_funcs *mf);
extern struct mstats _mstats (malloc_state *ms, malloc_funcs *mf);
extern void *_malloc_find_object_address (void *ptr, malloc_state *ms, malloc_funcs *mf);
