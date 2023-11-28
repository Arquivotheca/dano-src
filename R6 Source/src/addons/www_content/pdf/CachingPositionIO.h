#if ! defined CACHINGPOSITIONIO_INCLUDED
#define CACHINGPOSITIONIO_INCLUDED 1

#include <DataIO.h>

#define CACHE_ENTRY_BYTES 1
typedef uint8 cache_entry_t;
#define CACHE_ENTRY_EXP_BITS (CACHE_ENTRY_BYTES * 8 - 1)
#define CACHE_NULL ((1L << (CACHE_ENTRY_BYTES * 8)) - 1)
#define CACHE_ENTRIES (1L << CACHE_ENTRY_EXP_BITS)
#define CACHE_HASH_SIZE CACHE_ENTRIES * 4
#define CACHE_HASH_PRIME 509
#define CACHE_DEFAULT_SIZE 18	// 2^18 = 256k

// reads are cached, writes are (currently) not supported
class CachingPositionIO : public BPositionIO
{
public:
						CachingPositionIO(BPositionIO *source, int32 cacheLimitExpBits, size_t actualSize);// = CACHE_DEFAULT_SIZE);
						~CachingPositionIO();

	virtual	ssize_t		Read(void *buffer, size_t size);
	virtual	ssize_t		Write(const void *buffer, size_t size);
	
	virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
	virtual	ssize_t		WriteAt(off_t pos, const void *buffer, size_t size);

	virtual off_t		Seek(off_t position, uint32 seek_mode);
	virtual	off_t		Position() const;

	virtual status_t	SetSize(off_t size);

	void				FillCache(off_t offset, size_t size, const void *buffer, bool keepPartialPages = false);

private:
	void				PromoteEntryInLRU(int32 entry);
	cache_entry_t		CacheEntryForPage(int32 pagenum);
	cache_entry_t		CacheEntryForPage(uint8 *mempage);
	void				InitCache();
	bool				FindPage(int32 pagenum, uint8 **outPageBuf, int32 *outPageSize);
	status_t			ReadPages(int32 pagenum, int32 pagecount, uint8 **outPageBuf, int32 *outPageSize);
	ssize_t				ReadFullPageAt(off_t pos, uint8 *buffer, size_t size);
	cache_entry_t		MakeEntryMRU(int32 pagenum);
	void				DeleteEntry(cache_entry_t last = 0);
	void				*InitMalloc(void);
	void				*MallocPage(void);
	void				FreePage(void *page);
	void				CoalesceFreePages(void);
	cache_entry_t		MakeContiguousBlock(int32 pagenum, int32 pagecount, uint8 **outPageBuf);
#if defined(TEST_CACHINGPOSITIONIO)
	void				ValidateCacheEntries(void);
#endif

#if 0
	void DumpLRU(void);
#endif

	BPositionIO		*fSource;
	int32			fCacheLimit;
	int32			fPageExpBits;
	size_t			fActualSize;
	struct MallocFreeList
	{
		MallocFreeList *prev;
		MallocFreeList *next;
	}				*fMallocFreeList, *fMallocRoot;
	int32			fFreePages;

	// LRU and free entry lists overlap, since
	// an entry can't belong to both at the same time
	struct
	{
		cache_entry_t	next;
		cache_entry_t	prev;
	} fEntryList[CACHE_ENTRIES];
	cache_entry_t	fEntryFree;
	cache_entry_t	fLRUHead;
	cache_entry_t	fLRUTail;

	// last page information
	cache_entry_t	fLastPage;
	int32			fLastPageSize;

	// hash table structure
	cache_entry_t	fHash[CACHE_HASH_SIZE];	// index into fHashList
	struct
	{
		cache_entry_t	entry;				// index into fCacheEntry
		cache_entry_t	next;				// index into fHashList
	} fHashList[CACHE_ENTRIES];
	struct
	{
		int32	pagenum;
		uint8	*page;
	} fCacheEntry[CACHE_ENTRIES];
	cache_entry_t	fHashListFree;

#if defined(TEST_CACHINGPOSITIONIO)
public:
	void PrintLayout();

private:

#endif
};

#endif
