/*/  Metrowerks Standard Library  Version 1.2  07-May-96  /*/

/*
 *	pool_alloc.h
 *	
 *		Copyright © 1995-1996 Metrowerks, Inc.
 *		All rights reserved.
 */
 
#ifndef __pool_alloc__
#define __pool_alloc__

#pragma options align=mac68k

typedef signed long	tag_word;

typedef struct block_header {
	tag_word							tag;
	struct block_header *	prev;
	struct block_header *	next;
} block_header;

typedef struct list_header {
	block_header *	rover;
	block_header		header;
} list_header;

typedef unsigned long	mem_size;

typedef void *	(*sys_alloc_ptr)(mem_size size);
typedef void		(*sys_free_ptr)(void * ptr);

typedef struct pool_options{
	sys_alloc_ptr	sys_alloc_func;
	sys_free_ptr	sys_free_func;
	mem_size			min_heap_size;
	int						always_search_first;
} pool_options;

typedef struct mem_pool_obj {
	list_header		free_list;
	pool_options	options;
} mem_pool_obj;

#ifdef __cplusplus
extern "C" {
#endif

void		__init_pool_obj			(	mem_pool_obj * pool_obj															 	);
int			__pool_preallocate	(	mem_pool_obj * pool_obj, 								mem_size size	);
void		__pool_preassign		( mem_pool_obj * pool_obj, void * ptr,    mem_size size	);
void *	__pool_alloc				(	mem_pool_obj * pool_obj, 								mem_size size	);
void *	__pool_alloc_clear	(	mem_pool_obj * pool_obj, 								mem_size size	);
void *	__pool_realloc			(	mem_pool_obj * pool_obj, void * ptr, 		mem_size size	);
void		__pool_free					(	mem_pool_obj * pool_obj, void * ptr										);

void *	__sys_alloc(mem_size size);
void		__sys_free(void *ptr);

#ifdef __cplusplus
}
#endif

#pragma options align=reset

#endif
