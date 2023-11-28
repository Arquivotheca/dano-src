/******************************************************************************
/
/	File:			SupportDefs.h
/
/	Description:	Common type definitions.
/
/	Copyright 2001, Be Incorporated
/
******************************************************************************/

#ifndef __MEMORY_ADVISER
#define __MEMORY_ADVISER

#include <support/SupportDefs.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	B_FREE_CACHED= 1,	/* free cached data			*/
	B_FREE_ALL			/* free as much as possible	*/
} free_level;


typedef size_t	(*memory_adviser_free_func)(void *cookie, size_t sizeToFree, free_level level);
typedef void	(*memory_adviser_failure_func)(void *cookie, size_t sizeThatFailed, const char *requester);

extern int32 register_free_func(memory_adviser_free_func, void *cookie);
extern void  unregister_free_func(int32 free_func_token);

extern int32 register_failure_func(memory_adviser_failure_func, void *cookie);
extern void  unregister_failure_func(int32 failure_func_token);

extern bool  madv_reserve_memory(size_t bytes, char const *requester);
extern void  madv_finished_allocating(size_t size_reserved);

#ifdef __cplusplus
}
#endif

#endif
