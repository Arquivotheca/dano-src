#include <OS.h>
#include <string.h>
#include <stdio.h>

#include "Cache.h"


static uint8 TestEntry( cvContext *con, cvCacheEntry *id )
{
	if( con->in.Format != id->inFormat )
		return 0;
	if( con->in.Type != id->inType )
		return 0;
	if( con->in.SwapEndian != id->inSwapEndian )
		return 0;
	if( con->in.LsbFirst != id->inLsbFirst )
		return 0;
	if( con->out.Format != id->outFormat )
		return 0;
	if( con->out.Type != id->outType )
		return 0;
	if( con->out.SwapEndian != id->outSwapEndian )
		return 0;
	if( con->out.LsbFirst != id->outLsbFirst )
		return 0;
	if( con->transferMode.MapColor != id->MapColor )
		return 0;
	if( con->transferMode.MapStencil != id->MapStencil )
		return 0;
	return 1;
}


void cvCacheInit( cvContext *con, cvCache *cache )
{
	int ct;
	memset( cache, 0, sizeof( cvCache ));
	for( ct=0; ct<CV_CACHE_SIZE; ct++ )
		cache->entry[ct].areaID = -1;
}

int32 cvCacheFindEntry( cvContext *con, cvCache *cache )
{
	int32 ct;
	
	for( ct=0; ct<CV_CACHE_SIZE; ct++ )
	{
		if( TestEntry( con, &cache->entry[ct] ) )
		{
			cache->entry[ct].age = cache->clock++;
			return ct;
		}
	}
	return -1;
}

int32 cvCacheMakeEntry( cvContext *con, cvCache *cache, int32 size )
{
	int32 ct;
	int32 oldestIndex =0;
	uint32 oldestAge = 0; 
	
	for( ct=0; ct<CV_CACHE_SIZE; ct++ )
	{
		if( !cache->entry[ct].size )
		{
			oldestIndex = ct;
			break;
		}
	
		if( (cache->clock - cache->entry[ct].age) > oldestAge )
		{
			oldestAge = cache->clock - cache->entry[ct].age;
			oldestIndex = ct;
		}
	}
	
	if( cache->entry[oldestIndex].size < size )
	{
		int32 pages = (size + B_PAGE_SIZE-1) / B_PAGE_SIZE;
		
		if( cache->entry[oldestIndex].areaID >= 0 )
		{
			if( resize_area( cache->entry[oldestIndex].areaID, pages * B_PAGE_SIZE ) < B_OK )
				return -1;
		}
		else
		{
			if( (cache->entry[oldestIndex].areaID = create_area(
				"Bitflinger Cache", &cache->entry[oldestIndex].code,
				B_ANY_ADDRESS, pages * B_PAGE_SIZE, B_NO_LOCK,
				B_READ_AREA | B_WRITE_AREA )) < B_OK )
				return -1;
			cache->entry[oldestIndex].inFormat = con->in.Format;
			cache->entry[oldestIndex].inType = con->in.Type;
			cache->entry[oldestIndex].inSwapEndian = con->in.SwapEndian;
			cache->entry[oldestIndex].inLsbFirst = con->in.LsbFirst;
			cache->entry[oldestIndex].outFormat = con->out.Format;
			cache->entry[oldestIndex].outType = con->out.Type;
			cache->entry[oldestIndex].outSwapEndian = con->out.SwapEndian;
			cache->entry[oldestIndex].outLsbFirst = con->out.LsbFirst;
			cache->entry[oldestIndex].MapColor = con->transferMode.MapColor;
			cache->entry[oldestIndex].MapStencil = con->transferMode.MapStencil;
		}
		cache->entry[oldestIndex].size = pages * B_PAGE_SIZE;
	}
	
	return oldestIndex;
}


