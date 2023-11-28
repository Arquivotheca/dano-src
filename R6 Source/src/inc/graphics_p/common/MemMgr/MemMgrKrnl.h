#ifndef __MEMMGRKRNL_H__
#define __MEMMGRKRNL_H__

#include <graphics_p/common/MemMgr/MemMgr.h>

/* Core management function */
extern status_t __mem_Initialize( __mem_Area **area, uint32 pool_size, uint32 page_size, const char *area_name );

/* Clean up and orphaned allocations */
extern void __mem_Cleanup( __mem_Area *area );

/* Get a new client memory ID.  Returns < 0 for error */
extern int32 __mem_GetNewID( __mem_Area *area );

/* Release a memory ID and cleanup all its allocaitons. */
extern void __mem_RemoveIndex( __mem_Area *area, int32 index );


extern void __mem_Kprint_alloc( __mem_Allocation *alloc );


extern void __mem_SetSem( __mem_Area *area, uint32 key, __mem_AppNotify *not );

extern void __mem_ReleaseSems( __mem_Area *area );


#endif

