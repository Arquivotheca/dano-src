/* ++++++++++

   FILE:  BlockCache.cpp
   REVS:  $Revision: 1.8 $
   NAME:  peter
   DATE:  Thu May 18 10:40:25 PDT 1995

   Copyright (c) 1995 by Be Incorporated.  All Rights Reserved.

+++++ */

#ifndef _DEBUG_H
#include <Debug.h>
#endif

#ifndef _BLOCK_CACHE_H
#include "BlockCache.h"
#endif
#ifndef _MESSAGE_H
#include <Message.h>
#endif
#ifndef _STDLIB_H
#include <stdlib.h>
#endif

// -------------------------------------------------------------------- //

BBlockCache::BBlockCache(size_t cache_size, size_t blk_size, uint32 type)
	: fLock("block cache")
{
	fMark = -1;
	fBlkSize = blk_size;
	fCacheSize = cache_size;

	fCache = (void **) malloc(cache_size * sizeof(void *));

	if (type == B_OBJECT_CACHE) {
		fAlloc = (void *(*)(size_t))&(::operator new);
		fFree = &(::operator delete);
	} else {
		fAlloc = &malloc;
		fFree = &free;
	} 

#if DEBUG
	for (int i = 0; i < fCacheSize; i++)
		fCache[i] = NULL;
#endif
}

// -------------------------------------------------------------------- //

BBlockCache::~BBlockCache()
{
	// need to free whatever is left in the cache
	fLock.Lock();
	while (fMark >= 0)
		(*fFree)(fCache[fMark--]);

	free(fCache);

	ASSERT(fMark == -1);
	fLock.Unlock();
}

// -------------------------------------------------------------------- //

void *BBlockCache::Get(size_t size)
{
	void	*ptr;

	// if sizes don't match then we can't use the cache, because it contains
	// blocks of size fBlkSize.

	if (size == (size_t)fBlkSize) {
		fLock.Lock();
		
		if (fMark < 0)
			ptr = (*fAlloc)(fBlkSize);
		else
			ptr = fCache[fMark--];

		fLock.Unlock();
	} else {
		ptr = (*fAlloc)(size);
	}

	return ptr;
}

// -------------------------------------------------------------------- //

void BBlockCache::Save(void *ptr, size_t size)
{
	// if sizes don't match then we can't save ptr in the cache, because
	// the cache can only hold blocks of size fBlkSize.

	if (size == (size_t)fBlkSize) {
		fLock.Lock();

		if (fMark + 1 < fCacheSize)
			fCache[++fMark] = ptr;
		else
			(*fFree)(ptr);

		fLock.Unlock();
	} else {
		(*fFree)(ptr);
	}
}

/*-------------------------------------------------------------*/

BBlockCache::BBlockCache(const BBlockCache &) {}
BBlockCache &BBlockCache::operator=(const BBlockCache &) { return *this; }

/* ---------------------------------------------------------------- */

void BBlockCache::_ReservedBlockCache1() {}
void BBlockCache::_ReservedBlockCache2() {}

/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
/* ---------------------------------------------------------------- */
