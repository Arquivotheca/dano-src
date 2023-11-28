#include "CachingPositionIO.h"
#include <stdlib.h>
#include <assert.h>
#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <OS.h>
#include <Debug.h>

CachingPositionIO::CachingPositionIO(BPositionIO *source, int32 cacheLimitExpBits, size_t actualSize)
 : fSource(source), fCacheLimit(1L << cacheLimitExpBits),
   fPageExpBits(cacheLimitExpBits - CACHE_ENTRY_EXP_BITS), // == cache_size / entry_count
   fActualSize(actualSize),
   fMallocFreeList(0), fMallocRoot(0)
{
	actualSize = (actualSize + (1L << fPageExpBits) - 1) & ~((1L << fPageExpBits) - 1);
	if ((size_t)fCacheLimit > actualSize) fCacheLimit = actualSize;
	InitCache();
}

CachingPositionIO::~CachingPositionIO()
{
	free(fMallocRoot);
	delete fSource;
}

void 
CachingPositionIO::InitCache()
{
	int32 i;

	// build free/LRU list
	fEntryFree = 0;
	fEntryList[0].prev = CACHE_NULL;
	for(i = 0; i < CACHE_ENTRIES - 1; i++)
	{
		fEntryList[i].next = i + 1;
		fEntryList[i + 1].prev = i;
	}
	fEntryList[i].next = CACHE_NULL;
	fLRUHead = CACHE_NULL;
	fLRUTail = CACHE_NULL;

	// clear cached page pointers
	for(i = 0; i < CACHE_ENTRIES; i++)
	{
		fCacheEntry[i].page = 0;
		fCacheEntry[i].pagenum = -1;
	}

	// clear hash
	for(i = 0; i < CACHE_HASH_PRIME; i++)
		fHash[i] = CACHE_NULL;

	// build hash list
	fHashListFree = 0;
	for(i = 0; i < CACHE_ENTRIES - 1; i++)
	{
		fHashList[i].entry = CACHE_NULL;
		fHashList[i].next = i + 1;
	}
	fHashList[i].entry = CACHE_NULL;
	fHashList[i].next = CACHE_NULL;

	// last page in file
	fLastPage = CACHE_NULL;
	fLastPageSize = 0;
}

void *
CachingPositionIO::InitMalloc(void)
{
	// get some memory
	fMallocRoot = (MallocFreeList *)malloc(fCacheLimit);

	// or die trying
	if (fMallocRoot)
	{
		// prep the free list
		int32 chunkSize = 1L << fPageExpBits;
		int32 offset = fCacheLimit - chunkSize;
		MallocFreeList *last = 0;
		while (offset >= 0)
		{
			fMallocFreeList = (MallocFreeList *)((uint8*)fMallocRoot + offset);
			fMallocFreeList->next = last;
			if (last) last->prev = fMallocFreeList;
			last = fMallocFreeList;
			//printf("fMallocFreeList: %p, offset %.08lx\n", fMallocFreeList, offset);
			offset -= chunkSize;
		}
		fFreePages = fCacheLimit / chunkSize;
		fMallocFreeList->prev = 0;
	}
	return fMallocRoot;
}

void *
CachingPositionIO::MallocPage(void)
{
	void *result = 0;

	if (fMallocRoot || InitMalloc())
	{
		// any free memory?
		if (fMallocFreeList)
		{
			// note the free block
			result = (void *)fMallocFreeList;
			// remove block from free list
			fMallocFreeList = fMallocFreeList->next;
			if (fMallocFreeList) fMallocFreeList->prev = 0;
			// decrement free page count
			fFreePages--;
			//printf("fFreePages: %ld, fMallocFreeList %p\n", fFreePages, fMallocFreeList);
		}
	}
	return result;
}

void 
CachingPositionIO::FreePage(void *_page)
{
	MallocFreeList *page = (MallocFreeList *)_page;
	// free this page, keeping the free list sorted
	MallocFreeList *next = fMallocFreeList;
	MallocFreeList *last = 0;
	// while we have a list to walk, and the new page comes _after_ the current one
	while (next && (next < page))
	{
		// remember where we parked
		last = next;
		// take one step forward
		next = next->next;
	}
	// if last still null, remember the front of the list
	if (last == 0) fMallocFreeList = page;
	// otherwise, tuck this page in behind the last one
	else last->next = page;
	// in any case, set the next page after this one
	page->next = next;
	// update prev pointers
	if (next) next->prev = page;
	page->prev = last;
	// increment free page count
	fFreePages++;
}

cache_entry_t
CachingPositionIO::CacheEntryForPage(uint8 *mempage)
{
	// brute force search of fCacheEntry for a particular malloc page
	for (cache_entry_t i = 0; i < CACHE_ENTRIES; i++)
	{
		if (fCacheEntry[i].page == mempage)
			return i;
	}
	return CACHE_NULL;
}

void 
CachingPositionIO::CoalesceFreePages(void)
{
	// if we have two or more entries in the free list
	if (fFreePages > 1)
	{
		int32 chunkSize = 1L << fPageExpBits;
		MallocFreeList *last = fMallocFreeList;
		// gets us to the last item in the list, which we want
		while (last->next)
			last = last->next;

		// In theory, adding free pages to the list in sorted order
		// and handing them out lowest page first should tend to have
		// the bulk of the free pages congregate to the end of the list
		// making the coalescing of pages to the end of the list the
		// most efficient choice.  In theory.
		// In practice, we keep the cache full.  We only delete as many
		// pages as we need for the next read.  These pages could come
		// from anywhere, so we proably don't get the congregational
		// behaviour I imagined when I wrote the code.

		// while there are more pages
		while (last->prev)
		{
			// address we search for
			uint8 *src = (uint8*)last - chunkSize;
			// if not already contiguous
			if (src != (uint8*)(last->prev))
			{
				// take the first page
				uint8 *dest = (uint8*)MallocPage();
				ASSERT(dest != src);
				// copy the data
				memcpy(dest, src, chunkSize);
				// update the page cache
				cache_entry_t entry = CacheEntryForPage(src);
				ASSERT(entry != CACHE_NULL);
				fCacheEntry[entry].page = dest;
				// stuff this page in the free list
				FreePage(src);
			}
			// retreat to previous entry
			last = last->prev;
		}
	}
}

void
CachingPositionIO::PromoteEntryInLRU(int32 entry)
{
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	// upgrade page LRU position if the entry
	// isn't already the first one
	if(fEntryList[entry].prev != CACHE_NULL)
	{
		fEntryList[fEntryList[entry].prev].next = fEntryList[entry].next;
	
		// disconnect from next
		if(fEntryList[entry].next == CACHE_NULL)
		{
			fLRUTail = fEntryList[entry].prev;
			if(fLRUTail != CACHE_NULL)
				fEntryList[fLRUTail].next = CACHE_NULL;
		}
		else
			fEntryList[fEntryList[entry].next].prev = fEntryList[entry].prev;
	
		// connect at head
		fEntryList[entry].next = fLRUHead;
		fEntryList[fLRUHead].prev = entry;
		fEntryList[entry].prev = CACHE_NULL;
		fLRUHead = entry;
	}
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
}

#if defined(TEST_CACHINGPOSITIONIO)
void
CachingPositionIO::ValidateCacheEntries(void)
{
	for (int i = 0; i < CACHE_ENTRIES; i++)
	{
		if (fHashList[i].next == i)
			DEBUGGER("looped fHashList!");
		for (int j = 0; j < CACHE_ENTRIES; j++)
		{
			if ((i != j) && (fCacheEntry[i].pagenum != -1) && (fCacheEntry[i].pagenum == fCacheEntry[j].pagenum))
			{
				PrintLayout();
				DEBUGGER("duplicate cache entries!");
			}
		}
	}
		
}
#endif

cache_entry_t 
CachingPositionIO::CacheEntryForPage(int32 pagenum)
{
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	// choose starting position in hash table
	cache_entry_t	scan = fHash[pagenum % CACHE_HASH_PRIME];
	// until we find an unused hash slot
	while(scan != CACHE_NULL)
	{
		cache_entry_t entry = fHashList[scan].entry;
		ASSERT(entry != CACHE_NULL);

		if(fCacheEntry[entry].pagenum == pagenum)
		{
			// can't be null because at least the found page is in the cache
			ASSERT(fLRUHead != CACHE_NULL);
			ASSERT(fLRUTail != CACHE_NULL);
			break;
		}
		ASSERT(scan != fHashList[scan].next);
		// next bucket in hash chain
		scan = fHashList[scan].next;
	}
	// give up the goods
	return scan;
}

bool CachingPositionIO::FindPage(int32 pagenum, uint8 **outPageBuf, int32 *outPageSize)
{
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	cache_entry_t	entry = CacheEntryForPage(pagenum);

	*outPageSize = 0;
	*outPageBuf = 0;

	if (entry != CACHE_NULL)
	{
		*outPageBuf = fCacheEntry[entry].page;

		// return page size
		if(entry == fLastPage)
			*outPageSize = fLastPageSize;
		else
			*outPageSize = 1L << fPageExpBits;

		// upgrade page LRU position
		PromoteEntryInLRU(entry);
	}

#if defined(TEST_CACHINGPOSITIONIO)
	if (*outPageBuf == 0)
	{
		// exahustive search of cache entries
		for (int i = 0; i < CACHE_ENTRIES; i++)
			if (fCacheEntry[i].pagenum == pagenum)
				{
					DEBUGGER("FindPage() failed, but it really exists!");
				}
	}
	ValidateCacheEntries();
#endif
	return *outPageBuf ? true : false;
}

ssize_t 
CachingPositionIO::ReadFullPageAt(off_t pos, uint8 *buffer, size_t size)
{
	ssize_t just_read, total_read = 0;
	//printf("Starting ReadFullPageAt(%Ld, %p, %ld)\n", pos, buffer, size);
	while (size)
	{
		int retries = 3;
		do
		{
			just_read = fSource->ReadAt(pos, buffer, size);
		}
		while ((just_read <= 0) && (--retries > 0));
		if (just_read <= 0) break;
		pos += just_read;
		buffer += just_read;
		total_read += just_read;
		size -= just_read;
		//printf("   read %ld, %ld to go\n", just_read, size);
	}
	//printf("RFPA completes, total_read: %ld\n", total_read);
	return total_read ? total_read : B_ERROR;
}

cache_entry_t
CachingPositionIO::MakeEntryMRU(int32 pagenum)
{
#if defined(TEST_CACHINGPOSITIONIO)
	printf("MakeEntryMRU(%ld)\n", pagenum);
	ValidateCacheEntries();
#endif
	// take an entry off the free list and add it to the head of the LRU chain
	// returns entry, or CACHE_NULL if no entries on freelist
	cache_entry_t freeentry = fEntryFree;
	if (freeentry != CACHE_NULL)
	{
		// use entry
		cache_entry_t next = fEntryList[freeentry].next;
		fEntryFree = next;
		if(next != CACHE_NULL)
			fEntryList[next].prev = CACHE_NULL;
		fCacheEntry[freeentry].pagenum = pagenum;
		// no page 
		fCacheEntry[freeentry].page = (uint8 *)MallocPage();

		// will never grow more than list capacity
		cache_entry_t freelist = fHashListFree;
		ASSERT(fHashListFree != CACHE_NULL);

		// use hash entry
		int32 hash = pagenum % CACHE_HASH_PRIME;

		fHashListFree = fHashList[freelist].next;
		//ASSERT((fHash[hash] != CACHE_NULL) && (fHashList[freelist].next != fHash[hash]));
		//printf("fHash[hash]: %d, fHashList[freelist].next: %d\n", fHash[hash], fHashList[freelist].next);
		fHashList[freelist].next = fHash[hash];
		fHashList[freelist].entry = freeentry;
		fHash[hash] = freelist;

		// add page to LRU list
		fEntryList[freeentry].next = fLRUHead;
		fEntryList[freeentry].prev = CACHE_NULL;
		if (fLRUHead != CACHE_NULL)
			fEntryList[fLRUHead].prev = freeentry;
		fLRUHead = freeentry;
		if(fLRUTail == CACHE_NULL)
			fLRUTail = freeentry;
	}
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	return freeentry;
}

#if 0
void TwoHex(uint8 b, uint8 *buf)
{
	buf++;
	uint8 h = b & 0x0f;
	if (h > 9) h += 'A' - 10;
	else h += '0';
	*buf-- = h;
	b >>= 4;
	h = b & 0x0f;
	if (h > 9) h += 'A' - 10;
	else h += '0';
	*buf-- = h;
};
void DumpChunk(uint32 offset, uint8 *inbuf, int32 size)
{
	char buf[16 * 4 + 2];
	while (size)
	{
		memset(buf, ' ', sizeof(buf)-1);
		buf[sizeof(buf)-1] = '\0';
		for (int byte = 0; byte < 16 && size; byte++)
		{
			TwoHex(*inbuf, buf + (byte * 3));
			uint8 code = *inbuf++;
			buf[3 * 16 + 1 + byte] = ((code >= ' ') && (code <= 127)) ? code : '.';
			size--;
		}
		printf("%08lx  %s\n", offset, buf);
		offset += 16;
	}
	
}
#endif

cache_entry_t
CachingPositionIO::MakeContiguousBlock(int32 pagenum, int32 pagecount, uint8 **outPageBuf)
{
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	cache_entry_t	freeentry;

	if (pagecount > fFreePages)
	{
		// throw away enough pages to make this read work
		int toss = pagecount - fFreePages;
		while (toss--)
		{
			ASSERT(fLRUTail != CACHE_NULL);
			DeleteEntry(fLRUTail);
		}
	}
	ASSERT(fFreePages >= pagecount);
	// is this a "big" read?
	if (pagecount > 1)
		// make sure the free pages are contiguous
		CoalesceFreePages();

	// make and remember the first page
	freeentry = MakeEntryMRU(pagenum);

	// make and forget any remaining pages
	int32 nextpage = pagenum + 1;
	int32 morepages = pagecount - 1;
	while (morepages--)
		MakeEntryMRU(nextpage++);

	// read page from source PositionIO
	*outPageBuf = fCacheEntry[freeentry].page;
	
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	return freeentry;
}

status_t
CachingPositionIO::ReadPages(int32 pagenum, int32 pagecount, uint8 **outPageBuf, int32 *outPageSize)
{
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	status_t ret = B_ERROR;
	cache_entry_t	freeentry = MakeContiguousBlock(pagenum, pagecount, outPageBuf);

	// read page from source PositionIO
	size_t size = (1L << fPageExpBits) * pagecount;
	ssize_t read = ReadFullPageAt(pagenum << fPageExpBits, *outPageBuf, size);
	
	if (read < 0)
	{
		// note the error
		ret = read;
		// remove non-remembered pages from free list
		while (fLRUHead != freeentry)
			DeleteEntry(fLRUHead);
		// remove the first page
		DeleteEntry(fLRUHead);
	}
	else
	{
		//printf("read: %ld bytes from fSource\n", read);
		//DumpChunk(pagenum << fPageExpBits, *outPageBuf, read);
		*outPageSize = read;
		if (read != (ssize_t)size)
		{
			// there's only one "last page in file"
			ASSERT(fLastPage == CACHE_NULL);
			// big-read at end of file?
			if (pagecount > 1)
			{
				//printf("--- cleaning up after a truncated big read\n");
				// throw away any unused pages
				size_t delta = size - read;
				size_t chunkSize = (1L << fPageExpBits);
				while (delta >= chunkSize)
				{
					delta -= chunkSize;
					DeleteEntry(fLRUHead);
				}
				// adjust last page read size appropriately
				read = chunkSize - delta;
			}
			//printf("last page size: %ld\n", read);
			// the "las page" will always be at the head of the LRU
			fLastPage = fLRUHead;
			fLastPageSize = read;
		}
		ret = B_OK;
	}
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
	return ret;
}

#if 0
void
CachingPositionIO::DumpLRU(void)
{
	cache_entry_t scan = fLRUHead;
	printf("LRU list, head to tail\n");
	while (scan != CACHE_NULL)
	{
		printf(" %03d %05ld %p\n", (int)scan, fCacheEntry[scan].pagenum, fCacheEntry[scan].page);
		scan = fEntryList[scan].next;
	}
	printf("LRU list, tail to head\n");
	scan = fLRUTail;
	while (scan != CACHE_NULL)
	{
		printf(" %03d %05ld %p\n", (int)scan, fCacheEntry[scan].pagenum, fCacheEntry[scan].page);
		scan = fEntryList[scan].prev;
	}
}
#endif

void CachingPositionIO::DeleteEntry(cache_entry_t entry)
{
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
#if 0
	printf("entry: %d, fLRUTail: %d, fLRUHead %d\n", (int)entry, (int)fLRUTail, (int)fLRUHead);
	//if (entry == CACHE_NULL) PrintLayout();
#endif
	// we got here because there is an entry to be removed
	ASSERT(entry != CACHE_NULL);

	// disconnect entry from tail if we delete the end of the list
	if (entry == fLRUTail)
	{
		fLRUTail = fEntryList[entry].prev;
		if(fLRUTail != CACHE_NULL)
			fEntryList[fLRUTail].next = CACHE_NULL;
	}
	if (entry == fLRUHead)
	{
		DEBUGGER("entry == fLRUHead");
		fLRUHead = fEntryList[entry].next;
		if (fLRUHead != CACHE_NULL)
			fEntryList[fLRUHead].prev = CACHE_NULL;
	}
	// remove entry from hash
	int32			hash = fCacheEntry[entry].pagenum % CACHE_HASH_PRIME;
	cache_entry_t	scan = fHash[hash];
	cache_entry_t	last = CACHE_NULL;

#if defined(TEST_CACHINGPOSITIONIO)
	printf("hash chain for slot %ld: ", hash);
	while (scan != CACHE_NULL)
	{
		printf(" %d", scan);
		scan = fHashList[scan].next;
	}
	printf("\n");
	scan = fHash[hash];
#endif
	while(1)
	{
		// the entry hasn't been found yet,
		// but we know it's there
		ASSERT(scan != CACHE_NULL);

		if(fHashList[scan].entry == entry)
		{
			// remove entry from hash list
			if (last != CACHE_NULL)
				fHashList[last].next = fHashList[scan].next;
			else
				fHash[hash] = fHashList[scan].next;
			fHashList[scan].next = fHashListFree;
			fHashListFree = scan;
			break;
		}
		// no loops allowed!
		ASSERT(scan != fHashList[scan].next);
		last = scan;
		scan = fHashList[scan].next;
	}

	// free the page
	FreePage(fCacheEntry[entry].page);
	fCacheEntry[entry].page = 0;
	fCacheEntry[entry].pagenum = -1;
	// add entry to free list
	fEntryList[entry].prev = CACHE_NULL;
	fEntryList[entry].next = fEntryFree;
	if (fEntryFree != CACHE_NULL)
		fEntryList[fEntryFree].prev = entry;
	fEntryFree = entry;

	// remove last page info if this is last page
	if (entry == fLastPage)
		fLastPage = CACHE_NULL;
#if defined(TEST_CACHINGPOSITIONIO)
	ValidateCacheEntries();
#endif
}

ssize_t CachingPositionIO::Read(void *buffer, size_t size)
{
	ssize_t		ret;
	off_t		pos = fSource->Position();

	ret = ReadAt(pos, buffer, size);
	if(ret > 0)
		fSource->Seek(pos + ret, SEEK_SET);

	return ret;
}

ssize_t CachingPositionIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	ssize_t		ret = 0;
	status_t	err;
	int32		pagesize;
	int32		currentpage = pos >> fPageExpBits;
	uint8		*pagebuf;
	int32		pagebufoffs = pos - (currentpage << fPageExpBits);
	int32		copysize;
	int32		readpages;

	// prep the memory pool, if req'd
	if (!fMallocRoot && !InitMalloc()) return B_NO_MEMORY;
	//printf("ReadAt(%Ld, %p, %ld)\n", pos, buffer, size);
	//if (size > 10) debugger("non-one-byte-read!\n");
	//ASSERT(pos < fActualSize);
	// clamp to not read past EOF
	if (((size_t)pos + size) > fActualSize) size = fActualSize - (size_t)pos;
	int32 lastpage = (pos + size - 1) >> fPageExpBits;
	// while more bytes to read
	while(ret < (ssize_t)size)
	{
		// look for data in cache
		if(! FindPage(currentpage, &pagebuf, &pagesize))
		{
			readpages = currentpage;
			while ((readpages <= lastpage) && !FindPage(readpages, &pagebuf, &pagesize))
				readpages++;
			// determine run length of uncached pages
			readpages -= currentpage; // - 1;
			// limit reads to total number of pages in the cache
			if (readpages > (fCacheLimit >> fPageExpBits))
				readpages = (fCacheLimit >> fPageExpBits);
			// read some data
			err = ReadPages(currentpage, readpages, &pagebuf, &pagesize);
			if (err != B_OK)
			{
				//printf("++++ ReadPages(%ld, %ld) failed with reason: %s\n", currentpage, readpages, strerror(err));
				return err;
			}
		}
		else
			readpages = 1;
		copysize = pagesize - pagebufoffs;
		if(copysize > (ssize_t)size - ret)
			copysize = size - ret;
		if(copysize > 0)
		{
			memcpy((uint8 *)buffer + ret, pagebuf + pagebufoffs, copysize);
			ret += copysize;
			pagebufoffs = 0;
			currentpage += readpages;
		}
		// bail on partial underlying reads: EOF
		if(pagesize < ((1L << fPageExpBits) * readpages))
			break;
	}

	return ret;
}

ssize_t CachingPositionIO::Write(const void *, size_t )
{
	return B_ERROR;
}

ssize_t CachingPositionIO::WriteAt(off_t , const void *, size_t )
{
	return B_ERROR;
}

off_t CachingPositionIO::Seek(off_t position, uint32 seek_mode)
{
	return fSource->Seek(position, seek_mode);
}

off_t CachingPositionIO::Position() const
{
	return fSource->Position();
}

status_t CachingPositionIO::SetSize(off_t size)
{
	// TODO: semantics of SetSize?
	return fSource->SetSize(size);
}

void 
CachingPositionIO::FillCache(off_t pos, size_t size, const void *buffer, bool keepPartialPages)
{

	ssize_t		ret = 0;
	int32		pagesize;
	int32		currentpage = pos >> fPageExpBits;
	uint8		*pagebuf;
	int32		pagebufoffs = pos - (currentpage << fPageExpBits);
	int32		copysize;
	int32		readpages;

	// prep the memory pool, if req'd
	if (!fMallocRoot && !InitMalloc()) return;
	// make write end on page boundry if we don't keep partial pages
	if (!keepPartialPages)
	{
		size -= ((size_t)pos + size) & ((1 << fPageExpBits) - 1);
	}
	// don't calc lastpage until size known
	int32		lastpage = (pos + size - 1) >> fPageExpBits;

	//ASSERT(pos < fActualSize);

	// clamp to not write past EOF
	if (((size_t)pos + size) > fActualSize) size = fActualSize - (size_t)pos;
	// while more bytes to cache
	while(ret < (ssize_t)size)
	{
		// look for data in cache
		if(! FindPage(currentpage, &pagebuf, &pagesize))
		{
			readpages = currentpage;
			while ((readpages <= lastpage) && !FindPage(readpages, &pagebuf, &pagesize))
				readpages++;
			// determine run length of uncached pages
			readpages -= currentpage; // - 1;
			// limit reads to total number of pages in the cache
			if (readpages > (fCacheLimit >> fPageExpBits))
				readpages = (fCacheLimit >> fPageExpBits);
			MakeContiguousBlock(currentpage, readpages, &pagebuf);
			pagesize = readpages * (1 << fPageExpBits);
		}
		else
			readpages = 1;
		copysize = pagesize - pagebufoffs;
		if(copysize > (ssize_t)size - ret)
			copysize = size - ret;
		if(copysize > 0)
		{
			memcpy(pagebuf+pagebufoffs, (uint8 *)buffer + ret, copysize);
			ret += copysize;
			pagebufoffs = 0;
			currentpage += readpages;
		}
		// bail on partial underlying reads: EOF
		if(pagesize < ((1L << fPageExpBits) * readpages))
			break;
	}
}

#if defined(TEST_CACHINGPOSITIONIO)
#include <stdio.h>
#include <string.h>

void CachingPositionIO::PrintLayout()
{
	for(int i = 0; i < CACHE_HASH_PRIME; i++)
	{
		if(fHash[i] != CACHE_NULL)
		{
			printf("Hash[%04d]:", i);
			for(cache_entry_t scan = fHash[i]; scan != CACHE_NULL; scan = fHashList[scan].next)
				printf(" (%d)%ld", scan, fCacheEntry[fHashList[scan].entry].pagenum);
			printf("\n");
		}
	}

	uint8 conflict[256];
	memset(conflict, 0, 256);
	for(int i = 0; i < CACHE_HASH_PRIME; i++)
	{
		if(fHash[i] != CACHE_NULL)
		{
			if(conflict[fHash[i]])
				printf("conflicting hash entry %d at position %d\n", fHash[i], i);
			ASSERT(conflict[fHash[i]] == 0);
			conflict[fHash[i]] = 1;
		}
	}

	uint8 * check = (uint8 *)malloc(131072);
	memset(check, 0, 131072);
	for(int i = 0; i < CACHE_HASH_PRIME; i++)
	{
		if(fHash[i] != CACHE_NULL)
		{
			for(cache_entry_t scan = fHash[i]; scan != CACHE_NULL; scan = fHashList[scan].next)
			{
				int32 pagenum = fCacheEntry[fHashList[scan].entry].pagenum;
				if(check[pagenum])
					printf("duplicate %ld\n", pagenum);
				ASSERT(check[pagenum] == 0);
				check[pagenum] = 1;
			}
		}
	}
	free(check);
}
#endif


#if defined(TEST_CACHINGPOSITIONIO)

#include <File.h>
#include <stdio.h>
#include <OS.h>
#include <stdlib.h>
#include <time.h>

// compile with: gcc -DTEST_CACHINGPOSITIONIO CachingPositionIO.cpp -lbe

#define MAXFILESIZE 4*1024*1024
//#define MAXFILESIZE 727900
#define NUMREADS 10000
#define MAXREADSIZE 32768

main()
{
//	BFile f("test.file", O_RDWR | O_CREAT | O_TRUNC);
	BMallocIO f;

	f.SetSize(MAXFILESIZE);

	for(uint32 i = 0; i < MAXFILESIZE / 4; i++)
	{
		f.Write(&i, 4);
		if((i & 65535) == 0)
		{
			printf("\rCreating dataset... %d", i);
			fflush(stdout);
		}
	}
	printf("\rCreating dataset... done      \n");

	CachingPositionIO c(&f, 20, MAXFILESIZE);
	c.Seek(0, 0);

	struct readinfo
	{
		int32 seekpos;
		int32 datasize;
	};

	readinfo *r = (readinfo *)malloc(NUMREADS * sizeof(readinfo));
	uint32 *cachedbuf = (uint32 *)malloc(MAXREADSIZE);
	uint32 *realbuf = (uint32 *)malloc(MAXREADSIZE);

	//srand(time(0L));

	// build a stream of random read requests
	int32 j;
	for(j = 0; j < NUMREADS; j++)
	{
		r[j].seekpos = (int32)(((float)rand() / (float)RAND_MAX) * MAXFILESIZE);
		r[j].datasize = (int32)(((float)rand() / (float)RAND_MAX) * MAXREADSIZE);
	}

	printf("Performing consistency test... ");
	fflush(stdout);
	for(j = 0; j < NUMREADS; j++)
	{
		ssize_t ret1, ret2;
		ret1 = c.ReadAt(r[j].seekpos, cachedbuf, r[j].datasize);
		ret2 = f.ReadAt(r[j].seekpos, realbuf, r[j].datasize);
		if(ret1 != ret2 || memcmp(cachedbuf, realbuf, r[j].datasize) != 0)
			printf("read mismatch!\n");
	}
	printf("done\n");

	free(cachedbuf);
	free(realbuf);

	// expect malloc to complain when the CachingPositionIO attempts to delete BMallocIO f
	return 0;
}

#endif
