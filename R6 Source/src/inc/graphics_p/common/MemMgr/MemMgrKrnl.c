#include <graphics_p/common/MemMgr/MemMgr.h>
#include <KernelExport.h>
#include <stdio.h>
#include <string.h>

#include "jlock.h"
#include "MemMgrKrnl.h"

#define dprintf(x) dprintf x

status_t __mem_Initialize( __mem_Area **_area, uint32 pool_size, uint32 page_size, const char *area_name )
{
	char buf[256];
	uint8 *tmp;
	int32 ct,n;
	int32 size;
	int32 pages;
	area_id areaID;
	__mem_Area *area;

dprintf(( "MemMgrKrnl: __mem_Initialize \n" ));

	pages = (pool_size + page_size -1) / page_size;

	size = sizeof( __mem_Area );
	size += sizeof( int32 ) * 2 * pages;
	size += sizeof( __mem_Allocation ) * pages;

	sprintf( buf, "%s MemMgr area\n", area_name );

	n = (size + B_PAGE_SIZE - 1) / B_PAGE_SIZE;
	areaID = create_area ( buf, (void **) &area,
				   B_ANY_KERNEL_ADDRESS, n * B_PAGE_SIZE,
				   B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	
	if( areaID < B_OK )
	{
		return B_ERROR;
	}
	
dprintf(("MemMgrKrnl: __mem_Initialize - area = %p\n", area));

	area->areaID = areaID;
	tmp = (uint8 *)area;
	tmp += sizeof( __mem_Area );
	area->pageMap = (int32 *)tmp;
	tmp += sizeof( int32 ) * pages;
	area->pageIDs = (int32 *)tmp;
	tmp += sizeof( int32 ) * pages;
	area->allocs = (__mem_Allocation *)tmp;
	area->clock = 0;
	area->pageCount = pages;
	area->allocCount = pages;
	area->pageSize = page_size;
	area->baseOffset = 0;

	jlock_init( &area->listLock );

	for( ct=0; ct<area->pageCount; ct++ )
	{
		area->pageMap[ct] = -1;
		area->pageIDs[ct] = -1;
	}
	
	for( ct=0; ct<area->allocCount; ct++ )
	{
		area->allocs[ct].id = -1;
		area->allocs[ct].locked = 0;
		area->allocs[ct].state = __ALLOC_STATE_UNUSED__;
	}
	
	for( ct=0; ct<__GMEM_MAX_APPS__; ct++ )
	{
		area->lost[ct].used = 0;
		area->lost[ct].num = 0;
		area->apps[ct].start = -1;
		area->apps[ct].end = -1;
		area->apps[ct].team = -1;
	}
	
	*_area = area;
}

static uint8 checkID( __mem_Area *area, int32 id )
{
	if( (id >= 0) && (id < __GMEM_MAX_APPS__) )
	{
		return area->lost[id].used;
	}
	return 0;
}

void __mem_Cleanup( __mem_Area *area )
{
	int ct;
	
//dprintf(("MemMgrKrnl: __mem_Cleanup - cleaning pages \n" ));
	for( ct=0; ct<area->pageCount; ct++ )
	{
		if( !checkID(area, area->pageIDs[ct]) )
		{
//dprintf(("%x ", ct ));
			area->pageMap[ct] = -1;
			area->pageIDs[ct] = -1;
		}
	}

//dprintf(("MemMgrKrnl: __mem_Cleanup - cleaning allocs \n" ));
	for( ct=0; ct<area->allocCount; ct++ )
	{
		if( !checkID(area, area->allocs[ct].id) )
		{
//dprintf(("%x ", ct ));
			area->allocs[ct].id = -1;
			area->allocs[ct].state = __ALLOC_STATE_UNUSED__;
		}
	}
//dprintf(("MemMgrKrnl: __mem_Cleanup - done \n" ));
	
	if( !checkID(area, area->listLock+1) )
		jlock_unlock( &area->listLock );

}


int32 __mem_GetNewID( __mem_Area *area )
{
	int32 ct;
	
	for( ct=0; ct<__GMEM_MAX_APPS__; ct++ )
	{
		dprintf(("MemMgrKrnl: __mem_GetIndex - area->lost[%d].used = %x \n", ct, area->lost[ct].used  ));
		if( !area->lost[ct].used )
		{
			area->lost[ct].used = 1;
			area->lost[ct].num = 0;
			dprintf(("MemMgrKrnl: __mem_GetIndex  area=%x  returning %x \n", area, ct));
			return ct;
		}
	}
	
	dprintf(("MemMgrKrnl: __mem_GetIndex  returning -1 \n", ct));
	return -1;
}

void __mem_RemoveIndex( __mem_Area *area, int32 index )
{
	dprintf(("MemMgrKrnl: __mem_RemoveIndex - area=%x  index = %x\n", area, index));

	if( (index >= 0) && (index < __GMEM_MAX_APPS__))
	{
		area->lost[index].used = 0;
		__mem_Cleanup( area );
	}
	dprintf(("MemMgrKrnl: __mem_RemoveIndex - area->lost[%d].used = %x \n", index, area->lost[index].used  ));
}

void __mem_SetSem( __mem_Area *area, uint32 id, __mem_AppNotify *not )
{
	int32 ct;
	dprintf(("MemMgrKrnl: __mem_SetSem - id=%x   area=%p  not=%p \n", id, area, not ));

	if( (id >= 0) && (id < __GMEM_MAX_APPS__) )
		area->apps[id] = *not;
}

void __mem_ReleaseSems( __mem_Area *area )
{
	int32 ct;

	dprintf(("__mem_ReleaseSems: Enter \n" ));
	for( ct=0; ct<__GMEM_MAX_APPS__; ct++ )
	{
		if( area->apps[ct].start >= 0 )
		{
			dprintf(("__mem_ReleaseSems: releaseing 0x%x \n", area->apps[ct].start ));
			release_sem( area->apps[ct].start );
		}
	}

	dprintf(("__mem_ReleaseSems: All released OK \n" ));

	for( ct=0; ct<__GMEM_MAX_APPS__; ct++ )
	{
		if( area->apps[ct].end >= 0 )
		{
			dprintf(("__mem_ReleaseSems: acquiring 0x%x \n", area->apps[ct].end ));
			if( acquire_sem_etc( area->apps[ct].end, 1, 0, 3000000 ) == B_TIMEOUT )
			{
				dprintf(("__mem_ReleaseSems: acquiring 0x%x TIMEOUT, killing team 0x%x \n", area->apps[ct].end, area->apps[ct].team ));
				kill_team( area->apps[ct].team );
				__mem_RemoveIndex( area, ct );
			}
		}
	}
	
	
}

void __mem_Kprint_alloc( __mem_Allocation *alloc )
{
	kprintf( "    start & end  %x - %x \n", alloc->start, alloc->end );
	kprintf( "    baseAddress %p \n", alloc->address );
	kprintf( "    id %x \n", alloc->id );
	kprintf( "    lastBound %x \n", alloc->lastBound );
	kprintf( "    locked %x \n", alloc->locked );
	kprintf( "    state %x \n", alloc->state );
	kprintf( "    usr_ui1 %x \n", alloc->usr_ui1 );
	kprintf( "    usr_ui2 %x \n", alloc->usr_ui2 );
	kprintf( "    usr_vp1 %p \n", alloc->usr_vp1 );
	kprintf( "    usr_vp2 %p \n", alloc->usr_vp2 );
}

