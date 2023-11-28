/*
   Declarations for `malloc' and friends.
   Copyright 1990, 1991, 1992, 1993, 1995 Free Software Foundation, Inc.
		  Written May 1989 by Mike Haertel.
          Made Multi-thread safe and ported to the BeBox in Feb. 96
		  by dbg@be.com

This library is free software; you can redistribute it and/or
modify it under the terms of the GNU Library General Public License as
published by the Free Software Foundation; either version 2 of the
License, or (at your option) any later version.

This library is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
Library General Public License for more details.

You should have received a copy of the GNU Library General Public
License along with this library; see the file COPYING.LIB.  If
not, write to the Free Software Foundation, Inc., 675 Mass Ave,
Cambridge, MA 02139, USA.

   The author may be reached (Email) at the address mike@ai.mit.edu,
   or (US mail) as Mike Haertel c/o Free Software Foundation.
*/

#ifndef _MALLOC_H
#define _MALLOC_H	1

#include <unistd.h>

#ifdef _MALLOC_INTERNAL

#include <string.h>
#include <limits.h>

#endif	/* _MALLOC_INTERNAL.  */

#if __POWERPC__
# include <BeBuild.h>
#endif

#ifdef	__cplusplus
extern "C"
{
#endif


#include <stddef.h>

/*
   Allocate num_bytes of memory.
*/
extern void *malloc(size_t num_bytes);

/* Re-allocate the previously allocated block
   in old_ptr, making the new block new_size bytes long.
*/
extern void * realloc(void * old_ptr, size_t new_size);

/*
   Allocate num_elements elements of size bytes each, all initialized to 0.
*/
extern void *calloc(size_t num_elements, size_t size);

/*
   Free a block allocated by `malloc', `realloc' or `calloc'.
*/
extern void free(void *ptr);

/*
   Allocate num_bytes of memory at an address that is a multiple of alignment.
*/
extern void *memalign(size_t alignment, size_t num_bytes);

/*
   Allocate num_bytes on a page boundary.
*/
extern void *valloc(size_t num_bytes);


/*
   Given an address in the middle of a malloc'd object,
   return the address of the beginning of the object.
*/
extern void *malloc_find_object_address(void *ptr);

/*
   If not NULL, this function is called after each time
   `__morecore' is called to increase the data size.
*/
extern void (*__after_morecore_hook)(void);

/*
   Nonzero if `malloc' has been called and done its initialization.
*/
extern int __malloc_initialized;

/* Hooks for debugging versions.  */
extern void (*__malloc_initialize_hook)(void);
extern void (*__free_hook)(void *ptr);
extern void *(*__malloc_hook)(size_t size);
extern void *(*__realloc_hook)(void *ptr, size_t size);
extern void *(*__memalign_hook)(size_t size, size_t alignment);

/*
   Return values for `mprobe': these are the kinds of inconsistencies that
   `mcheck' enables detection of.
*/
enum mcheck_status
  {
    MCHECK_DISABLED = -1,	/* Consistency checking is not turned on.  */
    MCHECK_OK,			/* Block is fine.  */
    MCHECK_FREEDTWICE,	/* Block freed twice.  */
	MCHECK_FREE,		/* Block used after being freed. */
    MCHECK_HEAD,		/* Memory before the block was clobbered.  */
    MCHECK_TAIL			/* Memory after the block was clobbered.  */
  };

/*
   Activate a standard collection of debugging hooks.  This must be called
   before `malloc' is ever called.  ABORTFUNC is called with an error code
   (see enum above) when an inconsistency is detected.  If ABORTFUNC is
   null, the standard function prints on stderr and then calls `abort'.


   BEBOX USERS TAKE NOTE:
      If, in the shell, you set the environment variable MALLOC_DEBUG to any 
	  value ("export MALLOC_DEBUG=true" works well) then when you run your
	  app, malloc debugging will be turned on.  If you want to change what
	  happens when the mcheck code detects a problem, you should call 
	  mcheck() with a function pointer to the routine you want to handle
	  the error.
*/
extern int mcheck(void (*__abortfunc)(enum mcheck_status));

/*
   Check for aberrations in a particular malloc'd block.  You must have
   called `mcheck' already.  These are the same checks that `mcheck' does
   when you free or reallocate a block.
*/
extern enum mcheck_status mprobe(void *ptr);

/* Activate a standard collection of tracing hooks.  */
extern void mtrace(void);
extern void muntrace(void);

/* Statistics available to the user.  */
struct mstats
  {
    size_t bytes_total; /* Total size of the heap. */
    size_t chunks_used; /* Chunks allocated by the user. */
    size_t bytes_used;	/* Byte total of user-allocated chunks. */
    size_t chunks_free; /* Chunks in the free list. */
    size_t bytes_free;	/* Byte total of chunks in the free list. */
  };

/* Pick up the current statistics. */
extern struct mstats mstats(void);


/* Relocating allocator.  */

/* Allocate SIZE bytes, and store the address in *HANDLEPTR.  */
extern void *r_alloc(void **handleptr, size_t size);

/* Free the storage allocated in HANDLEPTR.  */
extern void r_alloc_free(void **handleptr);

/* Adjust the block at HANDLEPTR to be SIZE bytes long.  */
extern void *r_re_alloc(void **handleptr, size_t size);


#ifdef _MALLOC_INTERNAL
#include "malloc_internal.h"
#endif 


#ifdef	__cplusplus
}
#endif

#endif /* malloc.h  */
