/******************************************************************************
/
/	File:			BlockCache.h
/
/	Description:	BBlockCache class is a simple fixed-size
/					block caching mechanism.
/
/	Copyright 1993-98, Be Incorporated
/
******************************************************************************/

#ifndef _SUPPORT2_BLOCKCACHE_H
#define _SUPPORT2_BLOCKCACHE_H

#include <support2/Locker.h>

namespace B {
namespace Support2 {

/*-----------------------------------------------------------------*/
/*-----Allocation type --------------------------------------------*/
enum {
	B_OBJECT_CACHE = 0,
	B_MALLOC_CACHE = 1
};

/*--------------------------------------------------------------------*/
/*----- BBlockCache class --------------------------------------------*/

class BBlockCache {
public:
					BBlockCache(size_t cache_size,
								size_t block_size,
								uint32 type);
virtual				~BBlockCache();

		void		*Get(size_t block_size);
		void		Save(void *pointer, size_t block_size);

/*----- Private or reserved -----------------------------------------*/
private:

virtual	void		_ReservedBlockCache1();
virtual	void		_ReservedBlockCache2();

					BBlockCache(const BBlockCache &);
		BBlockCache	&operator=(const BBlockCache &);

		int			fCacheSize;
		void		**fCache;
		int			fMark;
		BLocker		fLock;
		int			fBlkSize;
		void		*(*fAlloc)(size_t size);
		void		(*fFree)(void *);
		uint32		_reserved[2];
};

/*-------------------------------------------------------------*/
/*-------------------------------------------------------------*/

} } // namespace B::Support2

#endif /* _SUPPORT2_BLOCKCACHE_H */
