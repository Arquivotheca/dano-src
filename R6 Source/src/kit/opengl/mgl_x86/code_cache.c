#include "code_cache.h"
#include <malloc.h>

typedef struct
{
	int64 key;
	void *buffer;
	int32 age;
	int32 size;
} entry;

typedef struct
{
	int32 count;
	entry entries[1];
} con;	



status_t __cc_init( void **context, int32 entries )
{
	con *c;
	int32 size;
	if( (entries < 1) || (entries > 4096) )
		return B_ERROR;
		
	size = sizeof( con ) + sizeof( entry ) * (entries-1);
	c = (con *)malloc( size );
	if( !c )
		return B_ERROR;
		
	memset( c, 0, size );
	c->count = entries;
	*context = c;
	return B_OK;
}

status_t __cc_uninit( void *context )
{
	con *c = (con *)context;
	
	free( c );
	return B_OK;
}

void * __cc_lookup( void *context, int64 key )
{
	con *c = (con *)context;
	void *ret = 0;
	int ct;

	for( ct=0; ct<c->count; ct++ )
	{
		c->entries[ct].age ++;
		if( c->entries[ct].key == key )
		{
			ret = c->entries[ct].buffer;
			c->entries[ct].age = 0;
		}
	}
	return ret;
}

status_t __cc_add( void *context, int64 key, int32 size, void **data )
{
	con *c = (con *)context;
	int ct;
	entry *e;

	// Replace the old key if present
	for( ct=0; ct<c->count; ct++ )
	{
		if( c->entries[ct].key == key )
			break;
	}
	
	if( ct >= c->count )
	{
		// Didn't find old key so choose the oldest one.
		int32 age = 0;
		int oldest = 0;
		
		for( ct=0; ct<c->count; ct++ )
		{
			if( c->entries[ct].buffer )
			{
				if( age <= c->entries[ct].age )
				{
					age = ++c->entries[ct].age;
					oldest = ct;
				}
			}
			else
			{
				oldest = ct;
				break;
			}
		}
		ct = oldest;
	}
	
	// ct = the entry to use at this point;
	e = &c->entries[ct];
	if( e->size < size )
	{
		free( e->buffer );
		e->buffer = malloc( size );
		if( !e->buffer )
			return B_ERROR;
		e->size = size;
	}
	*data = e->buffer;
	e->key = key;

	return B_OK;
}






