#ifndef __CACHE_H__
#define __CACHE_H__

#include "Bitflinger.h"


extern void cvCacheInit( cvContext *con, cvCache *cache );
extern int32 cvCacheFindEntry( cvContext *con, cvCache *cache );
extern int32 cvCacheMakeEntry( cvContext *con, cvCache *cache, int32 size );
extern void cvCacheEmpty( cvContext *con, cvCache *cache );




#endif
