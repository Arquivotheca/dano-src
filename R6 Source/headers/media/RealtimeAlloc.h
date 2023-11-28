/*******************************************************************************
/
/	File:			RealtimeAlloc.h
/
/   Description:    Allocation from separate "pools" of memory. Those pools
/					will be locked in RAM if realtime allocators are turned
/					on in the BMediaRoster, so don't waste this memory unless
/					it's needed. Also, the shared pool is a scarce resource,
/					so it's better if you create your own pool for your own
/					needs and leave the shared pool for BMediaNode instances
/					and the needs of the Media Kit.
/
/	Copyright 1998-99, Be Incorporated, All Rights Reserved
/
*******************************************************************************/

#if !defined(_REALTIME_ALLOC_H)
#define _REALTIME_ALLOC_H

#include <SupportDefs.h>

typedef struct rtm_pool rtm_pool;

#if _SUPPORTS_MEDIA_NODES

#if defined(__cplusplus)
extern "C" {
#endif

/* If out_pool is NULL, the default pool will be created if it isn't already. */
/* If the default pool is already created, it will return EALREADY. */
#if defined(__cplusplus)
status_t rtm_create_pool(rtm_pool ** out_pool, size_t total_size, const char * name=NULL);
#else
status_t rtm_create_pool(rtm_pool ** out_pool, size_t total_size, const char * name);
#endif
status_t rtm_delete_pool(rtm_pool * pool);
/* If NULL is passed for pool, the default pool is used (if created). */
void * rtm_alloc(rtm_pool * pool, size_t size);
status_t rtm_free(void * data);
status_t rtm_realloc(void ** data, size_t new_size);
status_t rtm_size_for(void * data);
status_t rtm_phys_size_for(void * data);

/* Return the default pool, or NULL if not yet initialized */
rtm_pool * rtm_default_pool();

#if defined(__cplusplus)
}
#endif

#else
#include <stdlib.h>
#if defined(__cplusplus)
inline status_t rtm_create_pool(rtm_pool ** out_pool, size_t total_size, const char * name = 0) { *out_pool = (rtm_pool *)4; return B_OK; }
#else
#define rtm_create_pool(a,b,c,d) ((*a = (rtm_pool *)4), B_OK)
#endif
#define rtm_delete_pool(p) B_OK
#define rtm_alloc(p,s) malloc(s)
#define rtm_free(p) (free(p),B_OK)
inline status_t rtm_realloc(void ** p, size_t s) { if (!p) return B_ERROR; void * _p = realloc(*p, s); if (!_p) return B_NO_MEMORY; *p = _p; return B_OK; }
#define rtm_size_for(d) %error%not%supported%
#define rtm_phys_size_for(d) %error%not%supported%
#define rtm_default_pool() ((rtm_pool *)4)
#endif


#if _SUPPORTS_MEDIA_NODES
#if defined(__cplusplus)
extern "C" {
#endif
rtm_pool * rtm_get_pool(rtm_pool * head, void * block);
status_t rtm_create_pool_etc(rtm_pool ** out_pool, size_t total_size, const char * name, uint32 flags, ...);
#if defined(__cplusplus)
}
#endif
#else
#define rtm_get_pool(h,b) ((rtm_pool *)4)
#if defined(__cplusplus)
extern "C" {
#endif
inline status_t rtm_create_pool_etc(rtm_pool ** out_pool, size_t total_size, const char * name, uint32 flags, ...) { if (!out_pool) return B_ERROR; *out_pool = (rtm_pool*)4; return B_OK; }
#if defined(__cplusplus)
}
#endif
#endif
enum {
	B_RTM_FORCE_LOCKED = 0x1,
	B_RTM_FORCE_UNLOCKED = 0x2
};

#endif // _REALTIME_ALLOC_H

