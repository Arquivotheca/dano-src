
#include "MemMgr.h"

#undef dprintf
#define dprintf(x) _kdprintf_ x
//#define dprintf(x) printf x

__mem_AreaDef * __mem_InitClient( int32 devfd )
{
	int32 memID;
	__mem_Area *area;
	area_id srcArea;
	area_id clone;
	__mem_AreaDef *def;
	
//dprintf (("MemMgr:  __mem_InitClient   devfd=%x \n", devfd ));
	
	if( ioctl( devfd, MEM_IOCTL_GET_MEMMGR_AREA, &srcArea, sizeof (srcArea)) < B_OK)
	{
		dprintf (("MemMgr:  ERROR getting memArea \n" ));
		return 0;
	}
	
	if( ioctl( devfd, MEM_IOCTL_GET_MEMMGR_ID, &memID, sizeof (memID)) < B_OK)
	{
		dprintf (("MemMgr:  ERROR getting memID \n" ));
		return 0;
	}

//dprintf (("MemMgr:  memID = %ld\n", memID ));
	
	clone = clone_area ("3dfx memMgr data: share", (void **) &area, B_ANY_ADDRESS, B_READ_AREA | B_WRITE_AREA, srcArea );
	if( clone < B_OK )
	{
		dprintf (("MemMgr:  ERROR cloning memory management area. \n" ));
		return 0;
	}

	def = (__mem_AreaDef *) malloc( sizeof( __mem_AreaDef ));
	if( !def )
	{
		dprintf (("MemMgr:  ERROR allocating AreaDef. \n" ));
		delete_area( clone );
		return 0;
	}

	def->memID = memID;
	def->area = area;
	def->clone = clone;
	def->devfd = devfd;
//dprintf (("MemMgr:  def = %p\n", def ));
	return def;
}

void __mem_CloseClient( __mem_AreaDef *ad )
{
	delete_area( ad->clone );
	free( ad );
}

int32 __mem_GetID( __mem_AreaDef *ad )
{
	return ad->memID;
}

static void mem_Free ( __mem_AreaDef *ad, __mem_Allocation *alloc )
{
	__mem_Area *area = ad->area;
	int ct;

//dprintf(( "__mem_Free ad=%p area=%p\n", ad, area ));

	if( alloc->state != __ALLOC_STATE_USED__ )
	{
		dprintf(( "MemMgr: __mem_Free attempt to free unused allocation, shouldn't happen. \n" ));
		return;
	}

//dprintf(( "__mem_Free 1 ad->memID %x \n", ad->memID ));
	if( area->lost[ad->memID].num < __GMEM_LOST_COUNT__ )
	{
		alloc->state = __ALLOC_STATE_LOST__;
		area->lost[ad->memID].list[ area->lost[ad->memID].num ] = alloc;
		area->lost[ad->memID].num++;
	}
	else
	{
		alloc->state = __ALLOC_STATE_UNUSED__;
		dprintf(( "MemMgr: __mem_Free Lost list overflow!, shouldn't happen. \n" ));
	}
		
//dprintf(( "__mem_Free 2 alloc->start %x \n", alloc->start ));
//dprintf(( "__mem_Free 2 alloc->end %x \n", alloc->end ));
	for (ct = alloc->start; ct < alloc->end; ct++)
		area->pageMap[ct] = -1;

//dprintf(( "__mem_Free 3 \n" ));
}

void __mem_Free ( __mem_AreaDef *ad, __mem_Allocation *alloc )
{
	__mem_Area *area = ad->area;
	jlock_lock( &area->listLock, ad->memID+1 );
	mem_Free ( ad, alloc );
	jlock_unlock( &area->listLock );
}

int __mem_CalcBumpCost ( __mem_Area *area, int32 base, int32 pages )
{
	float cost = 0;
	int ct, index, max;
	max = base + pages;

//dprintf(( "__mem_CalcBumpCost base=0x%x  pages=0x%x\n", base, pages ));
	for (ct = base; ct < max; ct++)
	{
		index = area->pageMap[ct];
		if ( index >= 0 )
		{
			float age = (float)(area->clock - area->allocs[index].lastBound);
//dprintf(( "__mem_CalcBumpCost age=0x%x  value=0x%x \n", (int)age, (int)area->allocs[index].value ));

			cost += (area->allocs[index].value * 1024) / (10 + age);
			ct = area->allocs[index].end;
		}
	}

	return cost;
}

__mem_Allocation * __mem_Allocate( __mem_AreaDef *ad, int32 size )
{
	__mem_Area *area = ad->area;
	int32 ct, temp, index, ct2;
	int32 pages = (size + area->pageSize - 1) / area->pageSize;
	int32 minCost = 0xffffff, minCostIndex = -1;

//dprintf(( "__mem_Allocation ad=%p  area=%p  size=0x%x\n", ad, area, size ));
	jlock_lock( &area->listLock, ad->memID+1 );
//dprintf(( "__mem_Allocation jlock_ok \n" ));
	for (ct = 0; ct < area->pageCount - pages -1; )
	{
//dprintf(( "__mem_Allocation ct=%x \n", ct ));
		for( ct2=0; (ct2<pages) && (ct+ct2 < area->pageCount); )
		{
//dprintf(( "__mem_Allocation ct2=%x \n", ct2 ));
			index = area->pageMap[ct+ct2];
			if ( (index >= 0) && (area->allocs[index].locked) )
			{
//dprintf(( "Skipping locked area  index=0x%x  start=0x%x  end=0x%x  size=0x%x\n", index, area->allocs[index].start, area->allocs[index].end, size ));
				ct = area->allocs[index].end;
				ct2 = 0;
				continue;
			}
			ct2++;
		}

		if( ct >= (area->pageCount - pages -1) )
			continue;
		
//dprintf(( "__mem_Allocation 3 \n" ));
		temp = __mem_CalcBumpCost (area, ct, pages );
//dprintf(( "__mem_Allocation 4  bumpCost = %ld \n", temp ));
		if (temp < minCost)
		{
			minCost = temp;
			minCostIndex = ct;
		}
		
//dprintf(( "__mem_Allocation 5 minCost=%ld  minCostIndex=%ld \n", minCost, minCostIndex ));
		index = area->pageMap[ct];
		if (index >= 0)
			ct = area->allocs[index].end;
		else
			ct++;
		if( temp == 0 )
			break;
	}
//dprintf(( "__mem_Allocation  minCostIndex = %x \n", minCostIndex ));

	// Check to make sure we found memory.
	if( minCostIndex < 0 )
	{
//dprintf(( "Not enough unlocked memory \n" ));
		jlock_unlock( &area->listLock );
		return 0;
	}

	for (ct = minCostIndex; ct < minCostIndex + pages; ct++)
	{
		if (area->pageMap[ct] >= 0)
			mem_Free (ad, &area->allocs[area->pageMap[ct]] );
	}

//dprintf(( "__mem_Allocation  after free \n" ));
	for( ct=0; ct<area->allocCount; ct++ )
	{
		if ( area->allocs[ct].state == __ALLOC_STATE_UNUSED__ )
			break;
	}
	
//dprintf(( "__mem_Allocation  alloc = %x \n", ct ));
	if( ct == area->allocCount )
	{
		printf( "MemMgr: Allocs exausted \n" );
	}
	
	for( temp=0; temp<pages; temp++ )
	{
		area->pageMap[temp+minCostIndex] = ct;
		area->pageIDs[temp+minCostIndex] = ad->memID;
	}
	
	area->allocs[ct].start = minCostIndex;
	area->allocs[ct].address = area->allocs[ct].start * area->pageSize + area->baseOffset;
	area->allocs[ct].end = area->allocs[ct].start + pages;
	area->allocs[ct].state = __ALLOC_STATE_USED__;
	area->allocs[ct].value = size;
	area->allocs[ct].id = ad->memID;
	area->allocs[ct].lastBound = area->clock++;
	area->allocs[ct].index = ct;
	area->allocs[ct].locked = 0;
	area->allocs[ct].priority = 1.0;

	area->allocs[ct].usr_ui1 = 0;
	area->allocs[ct].usr_ui2 = 0;
	area->allocs[ct].usr_vp1 = 0;
	area->allocs[ct].usr_vp2 = 0;
	
	jlock_unlock( &area->listLock );
	
	return &area->allocs[ct];
}

void __mem_FreeAll( __mem_AreaDef *ad, uint8 maxLockLevel  )
{
	__mem_Area *area = ad->area;
	int32 ct;

	jlock_lock( &area->listLock, ad->memID+1 );
	for (ct = 0; ct < area->pageCount; ct++ )
	{
		if( area->allocs[ct].state &&
			(area->allocs[ct].id == ad->memID) &&
			(area->allocs[ct].locked <= maxLockLevel ))
			
		{
			mem_Free ( ad, &area->allocs[ct] );
		}
	}
	jlock_unlock( &area->listLock );

}

void __mem_Touch( __mem_AreaDef *ad, __mem_Allocation *alloc )
{
	__mem_Area *area = ad->area;
	alloc->lastBound = area->clock++;
}

void __mem_Lock( __mem_AreaDef *ad )
{
	__mem_Area *area = ad->area;
	jlock_lock( &area->listLock, ad->memID+1 );
}

void __mem_Unlock( __mem_AreaDef *ad )
{
	__mem_Area *area = ad->area;
	jlock_unlock( &area->listLock );
}

__mem_LostList * __mem_GetLostList( __mem_AreaDef *ad )
{
	__mem_Area *area = ad->area;
	return &area->lost[ad->memID];
}


void __mem_print_alloc( __mem_Allocation *alloc )
{
	printf( "__mem_Allocation %p  index=%i \n", alloc, alloc->index );
	printf( "    start & end  %x - %x \n", alloc->start, alloc->end );
	printf( "    baseAddress %p \n", alloc->address );
	printf( "    id %x \n", alloc->id );
	printf( "    value %f \n", alloc->value );
	printf( "    priority %f \n", alloc->priority );
	printf( "    lastBound %x \n", alloc->lastBound );
	printf( "    locked %x \n", alloc->locked );
	printf( "    state %x \n", alloc->state );
}

void __mem_dprint_alloc( __mem_Allocation *alloc )
{
/*
	_kdprintf_( "__mem_Allocation %p  index=%i \n", alloc, alloc->index );
	_kdprintf_( "    start & end  %x - %x \n", alloc->start, alloc->end );
	_kdprintf_( "    baseAddress %p \n", alloc->address );
	_kdprintf_( "    id %x \n", alloc->id );
	_kdprintf_( "    lastBound %x \n", alloc->lastBound );
	_kdprintf_( "    locked %x \n", alloc->locked );
	_kdprintf_( "    state %x \n", alloc->state );
*/
}




