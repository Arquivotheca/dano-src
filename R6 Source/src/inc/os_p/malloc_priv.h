
#ifndef _MALLOC_PRIV_H
#define _MALLOC_PRIV_H

/* prototypes for private malloc calls used by libroot, etc.*/
void cfree(void *ptr);
void _m_release_sem(malloc_state *);
void _m_acquire_sem(malloc_state *);
void __lock_gen_malloc();
void __unlock_gen_malloc();
void _init_gen_malloc(void);
void _cleanup_gen_malloc(void);
void __fork_gen_malloc(void);

void * _realloc_internal(void *, size_t, malloc_state *, malloc_funcs *);
void _free_internal (void *, malloc_state *, malloc_funcs *);
void *_memalign_internal (size_t alignment, size_t size, malloc_state *ms,
					 malloc_funcs *mf);

enum mcheck_status _mprobe (void *, malloc_funcs *);

#endif
