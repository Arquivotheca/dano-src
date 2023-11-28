#ifndef __CODE_CACHE_H__
#define __CODE_CACHE_H__

#include <OS.h>


status_t __cc_init( void **context, int32 entries );
status_t __cc_uninit( void *context );

void * __cc_lookup( void *context, int64 key );
status_t __cc_add( void *context, int64 key, int32 size, void **data );






#endif

