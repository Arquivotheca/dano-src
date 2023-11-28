#include "HashCache.h"
#include <Debug.h>
#include <stdio.h>

#define CACHE_NULL ((cache_index_t)~0)

HashCache::HashCache(cache_index_t cacheItems)
	: fCacheSize(cacheItems)
{
	// size the hash table
	// a more generic class would probably use something more sophisticated
	if (fCacheSize == CACHE_NULL) fCacheSize = CACHE_NULL-1;
	fHashPrime = ((fCacheSize << 1) < 509) ? 509 : 1021;
	fHashTable = new cache_index_t[fHashPrime];
	fHashEntries = new hash_entry[fCacheSize];
	fCacheEntries = new cache_entry[fCacheSize];
	ResetCache();
}


HashCache::~HashCache()
{
	MakeEmpty();
	delete fCacheEntries;
	delete fHashEntries;
	delete fHashTable;
}

void 
HashCache::ResetCache(void)
{
	fFreeList = 0;
	fHashFreeList = 0;
	fLRUHead = CACHE_NULL;
	fLRUTail = CACHE_NULL;

	for (uint32 i = 0; i < fHashPrime; i++)
	{
		fHashTable[i] = CACHE_NULL;
	}
	for (uint32 i = 0; i < fCacheSize; i++)
	{
		// empty hash entries
		fHashEntries[i].entry = CACHE_NULL;
		fHashEntries[i].next = i+1;
		// build free list
		fCacheEntries[i].prev = i-1;
		fCacheEntries[i].next = i+1;
		// empty cache
		fCacheEntries[i].key = ~0;
		fCacheEntries[i].value = 0;	
	}
	// fix up free lists
	fCacheEntries[0].prev = CACHE_NULL;
	fCacheEntries[fCacheSize-1].next = CACHE_NULL;
	fHashEntries[fCacheSize-1].next = CACHE_NULL;
#ifndef NDEBUG
	printf("\nHashCache hit rate %lu/%lu = %.3f\n", fHits, fTries, ((double)fHits * 100.0) / (double)fTries);
	fTries = fHits = 0;
#endif
}

HashCache::cache_index_t
HashCache::CacheEntryIndexFor(uint32 key)
{
#ifndef NDEBUG
	Validate();
#endif
	// find an entry in the table
	cache_index_t index = fHashTable[key % fHashPrime];
	while (index != CACHE_NULL)
	{
		if (fCacheEntries[fHashEntries[index].entry].key == key)
		{
			// our result
			index = fHashEntries[index].entry;
			break;
		}
		index = fHashEntries[index].next;
	}
	return index;
}

void 
HashCache::Promote(cache_index_t index)
{
	ASSERT(index != CACHE_NULL);
	ASSERT(index < fCacheSize);

	// easy case?
	if (index == fLRUHead) return;
	
	cache_entry *entry = fCacheEntries + index;
	// if item at tail of list, note new tail
	if (index == fLRUTail) fLRUTail = entry->prev;	
	// extract from list
	if (entry->prev != CACHE_NULL) fCacheEntries[entry->prev].next = entry->next;
	if (entry->next != CACHE_NULL) fCacheEntries[entry->next].prev = entry->prev;
	// insert at head
	entry->next = fLRUHead;
	entry->prev = CACHE_NULL;
	if (fLRUHead != CACHE_NULL) fCacheEntries[fLRUHead].prev = index;
	// make fListEntries[index] the MRU entry
	fLRUHead = index;
	// first item added to list?
	if (fLRUTail == CACHE_NULL) fLRUTail = index;
#ifndef NDEBUG
	Validate();
#endif
}

void
HashCache::RemoveFromHash(uint32 key)
{
#ifndef NDEBUG
	Validate();
#endif
	// find an entry in the table
	cache_index_t *prev = fHashTable + (key % fHashPrime);
	cache_index_t index = *prev;
	while (index != CACHE_NULL)
	{
		if (fCacheEntries[fHashEntries[index].entry].key == key)
		{
			// fix up the chain
			*prev = fHashEntries[index].next;
			fHashEntries[index].next = fHashFreeList;
			fHashEntries[index].entry = CACHE_NULL;
			fHashFreeList = index;
			ASSERT(fHashFreeList < fCacheSize);// || (fHashFreeList == CACHE_NULL));
			break;
		}
		prev = &(fHashEntries[index].next);
		index = fHashEntries[index].next;
	}
	ASSERT(fHashFreeList < fCacheSize);
#ifndef NDEBUG
	Validate();
#endif
}

void
HashCache::DeleteLRUTail(void)
{
#ifndef NDEBUG
	Validate();
#endif
	cache_index_t entry = fLRUTail;
	// dispose of vallue
	DisposeEntry(fCacheEntries[entry].value);
	// extract from tail of list
	fLRUTail = fCacheEntries[entry].prev;
	if (fLRUTail != CACHE_NULL)
		fCacheEntries[fLRUTail].next = CACHE_NULL;
	else
		fLRUHead = CACHE_NULL;
	// add to free list
	fCacheEntries[entry].next = fFreeList;
	fCacheEntries[entry].prev = CACHE_NULL;
	if (fFreeList != CACHE_NULL) fCacheEntries[fFreeList].prev = entry;
	fFreeList = entry;
	// update hash buckets
	RemoveFromHash(fCacheEntries[entry].key);
	// mark entry free (must happen after RemoveFromHash())
	fCacheEntries[entry].key = ~0;
	fCacheEntries[entry].value = 0;
#ifndef NDEBUG
	Validate();
#endif
}

void 
HashCache::EnsureFreeEntries(cache_index_t items)
{
#ifndef NDEBUG
	Validate();
#endif
	cache_index_t entry = fFreeList;
	// reduce count by number of items in free list
	while (items && (entry != CACHE_NULL))
	{
		entry = fCacheEntries[entry].next;
		items--;
	}
	// free up items
	while (items && (fLRUTail != CACHE_NULL))
	{
		// delete the last item
		DeleteLRUTail();
		// next item
		items--;
	}
	ASSERT(fFreeList != CACHE_NULL);
#ifndef NDEBUG
	Validate();
#endif
}

void *
HashCache::Intern(uint32 key, void *payload)
{
#ifndef NDEBUG
	Validate();
#endif
	void *result = 0;
	// find an entry in the table
	cache_index_t index = CacheEntryIndexFor(key);
	// if it already exists
	if (index != CACHE_NULL)
	{
		// return result to caller
		result = fCacheEntries[index].value;
		// store the new result
		fCacheEntries[index].value = payload;
	}
	else
	{
		// free up an entry, if we need to
		EnsureFreeEntries(1);
		ASSERT(fFreeList != CACHE_NULL);
		ASSERT(fFreeList < fCacheSize);
		ASSERT(fHashFreeList != CACHE_NULL);
		ASSERT(fHashFreeList < fCacheSize);
		// get item from free list
		index = fFreeList;
		fFreeList = fCacheEntries[fFreeList].next;
		if (fFreeList != CACHE_NULL) fCacheEntries[fFreeList].prev = CACHE_NULL;
		// issolate
		fCacheEntries[index].next = CACHE_NULL;
		// fCacheEntries[index].prev = CACHE_NULL; // already CACHE_NULL from free list
		// populate entry
		fCacheEntries[index].key = key;
		fCacheEntries[index].value = payload;
		// intern in hash table
		cache_index_t hash = key % fHashPrime; // table entry
		cache_index_t hashIndex = fHashTable[hash]; // start of hash chain
		fHashTable[hash] = fHashFreeList; // link new item to start of chain
		fHashEntries[fHashFreeList].entry = index; // make new item point to cache entry
		cache_index_t oldFree = fHashFreeList;
		fHashFreeList = fHashEntries[fHashFreeList].next; // adjust start of free list
		ASSERT((fHashFreeList < fCacheSize) || (fHashFreeList == CACHE_NULL));
		fHashEntries[oldFree].next = hashIndex; // link old items after new one
	}
	// Promote this entry
	Promote(index);
	//printf("Intern(%lu)\n", key);
#ifndef NDEBUG
	Validate();
#endif
	// give up the goods
	return result;
}

void *
HashCache::Find(uint32 key)
{
#ifndef NDEBUG
	Validate();
	fTries++;
#endif
	void *result = 0;
	//printf("Find(%lu)\n", key);
	// find an entry in the table
	cache_index_t index = CacheEntryIndexFor(key);
	// if we found it
	if (index != CACHE_NULL)
	{
		// promote it to the MRU status
		Promote(index);
		// return result to caller
		result = fCacheEntries[index].value;
#ifndef NDEBUG
		fHits++;
#endif
	}
#ifndef NDEBUG
	Validate();
	if ((fTries % 100) == 0)
		printf("\nHashCache hit rate %lu/%lu = %.3f\n", fHits, fTries, ((double)fHits * 100.0) / (double)fTries);
#endif
	return result;
}

void 
HashCache::MakeEmpty(void)
{
#ifndef NDEBUG
	Validate();
	cache_index_t count = 0;
#endif
	// walk the LRU, disposing of entries
	cache_index_t index = fLRUHead;
	while (index != CACHE_NULL)
	{
		DisposeEntry(fCacheEntries[index].value);
		index = fCacheEntries[index].next;
#ifndef NDEBUG
		count++;
#endif
	}
	// now reset the state
	ResetCache();
#ifndef NDEBUG
	printf("HashCache had %lu entries\n", (uint32)count);
	Validate();
#endif
}

void 
HashCache::DisposeEntry(void *)
{
	// do nothing, gracefully
}

#ifndef NDEBUG
void 
HashCache::Validate()
{
#if 0
	// cache entry data
	for (cache_index_t i = 0; i < (fCacheSize - 1); i++)
		for (cache_index_t j = i + 1; j < fCacheSize; j++)
			if ((fCacheEntries[i].key != (uint32)~0) && (fCacheEntries[i].key == fCacheEntries[j].key))
			{
				DEBUGGER("duplicate cache entries!");
			}
	// cache free list and LRU lists
	cache_index_t count = fCacheSize;
	cache_index_t index = fFreeList;
	cache_index_t free_last = CACHE_NULL;
	while (index != CACHE_NULL)
	{
		free_last = index;
		index = fCacheEntries[index].next;
		count--;
	}
	index = fLRUHead;
	while (index != CACHE_NULL)
	{
		index = fCacheEntries[index].next;
		count--;
	}
	if (count) DEBUGGER("items missing from forward cache walk");
	// cache free list and LRU lists
	count = fCacheSize;
	index = free_last;
	while (index != CACHE_NULL)
	{
		index = fCacheEntries[index].prev;
		count--;
	}
	index = fLRUTail;
	while (index != CACHE_NULL)
	{
		index = fCacheEntries[index].prev;
		count--;
	}
	if (count) DEBUGGER("items missing from backward cache walk");
#endif
}
#endif
