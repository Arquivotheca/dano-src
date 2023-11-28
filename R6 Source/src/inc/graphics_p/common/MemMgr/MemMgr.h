#ifndef __MEMMGR_H__
#define __MEMMGR_H__

#include <OS.h>
#include <graphics_p/common/MemMgr/jlock.h>

#ifdef __cplusplus
extern "C" {
#endif

#define __GMEM_LOST_COUNT__ 1024
#define __GMEM_MAX_APPS__ 16

#define __ALLOC_STATE_UNUSED__ 0
#define __ALLOC_STATE_USED__ 1
#define __ALLOC_STATE_LOST__ 2

enum release_commands {
	MEM_COMMAND_FREE_MEMORY,
	MEM_COMMAND_MEMORY_AVAILABLE,
};

typedef struct __mem_AllocationRec
{
	uint32 address;			// area->baseOffset + sart*pageSize
	uint32 usr_ui1;			// User storage
	uint32 usr_ui2;			// User storage
	void * usr_vp1;			// User storage
	void * usr_vp2;			// User storage

	int32 start;			// First page
	int32 end;				// Last page +1
	uint32 id;				// ID of owening client
	float value;			// eviction cost
	float priority;			// residency priority
	uint32 lastBound;		// clock at last use
	int32 index;			// index into allocation map
	uint8 locked;			// lock level, 0=unlocked
	uint8 state;			// 0=unused, 1=used, 2=used but lost.
} __mem_Allocation;

typedef struct __mem_LostListRec
{
	int32 num;
	__mem_Allocation *list[__GMEM_LOST_COUNT__];
	uint8 used;
} __mem_LostList;

typedef struct __mem_AppNotifyRec
{
	sem_id start;
	sem_id end;
	team_id team;
} __mem_AppNotify;

typedef struct __mem_AreaRec {
	area_id areaID;
	int32 *pageMap;
	int32 *pageIDs;
	__mem_Allocation *allocs;
	uint32 clock;
	int32 pageCount;
	int32 allocCount;
	int32 pageSize;
	uint32 baseOffset;
	
	jlock listLock;
	
	int32 memIDs[__GMEM_MAX_APPS__];
	__mem_LostList lost[__GMEM_MAX_APPS__];
	__mem_AppNotify apps[__GMEM_MAX_APPS__];
	
} __mem_Area;	

typedef struct __mem_AreaDefRec {
	int32 devfd;
	__mem_Area *area;
	int32 memID;
	area_id clone;
} __mem_AreaDef;



/* Client routines */
extern __mem_AreaDef * __mem_InitClient( int32 devfd );
extern void __mem_CloseClient( __mem_AreaDef *df );
extern int32 __mem_GetID( __mem_AreaDef *df );

extern __mem_Allocation * __mem_Allocate( __mem_AreaDef *df, int32 size );

extern void __mem_Free( __mem_AreaDef *df, __mem_Allocation *alloc );
extern void __mem_FreeAll( __mem_AreaDef *df, uint8 maxLockLevel );
extern void __mem_Touch( __mem_AreaDef *df, __mem_Allocation *alloc );
extern void __mem_print_alloc( __mem_Allocation *alloc );
extern void __mem_dprint_alloc( __mem_Allocation *alloc );

extern void __mem_Lock( __mem_AreaDef *df );
extern __mem_LostList * __mem_GetLostList( __mem_AreaDef *df );
extern void __mem_Unlock( __mem_AreaDef *fd );

extern __mem_Allocation * __mem_GetLost( __mem_AreaDef *df );

#ifdef __cplusplus
}
#endif


#endif

