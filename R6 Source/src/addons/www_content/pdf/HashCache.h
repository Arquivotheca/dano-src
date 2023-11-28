#if !defined(__HASHCACHE_H__)
#define __HASHCACHE_H__

#include <SupportDefs.h>

class HashCache {
protected:
typedef uint16 cache_index_t;

				HashCache(cache_index_t cacheItems);
virtual			~HashCache();
void *			Intern(uint32 key, void *payload);
void *			Find(uint32 key);
void			MakeEmpty(void);
void			EnsureFreeEntries(cache_index_t items);

virtual void	DisposeEntry(void *payload);	// callback after item reoved from cache

private:
void			RemoveFromHash(uint32 key);
void			DeleteLRUTail(void);
cache_index_t	CacheEntryIndexFor(uint32 key);
void			ResetCache(void);
void			Promote(cache_index_t index);
#ifndef NDEBUG
void			Validate();
uint32			fTries;
uint32			fHits;
#endif
	struct hash_entry
	{
		cache_index_t	entry;	// index into fCacheEntries
		cache_index_t	next;	// index into fHashEntries
	};
	struct cache_entry
	{
		uint32			key;
		void			*value;
		cache_index_t	prev;		// LRU previous (more recently used)
		cache_index_t	next;		// LRU next (less recently used)
	};
	uint32			fCacheSize;
	uint32			fHashPrime;
	cache_index_t	*fHashTable;
	hash_entry		*fHashEntries;
	cache_entry		*fCacheEntries;
	cache_index_t	fFreeList;
	cache_index_t	fLRUHead;
	cache_index_t	fLRUTail;
	cache_index_t	fHashFreeList;
};

#endif
