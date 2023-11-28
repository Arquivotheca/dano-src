// ---------------------------------------------------------------------------
/*
	profile.h
	
	Copyright (c) 2000 Be Inc. All Rights Reserved.
	
	Author:	John R. Dance
			7 December 2000
	
*/
// ---------------------------------------------------------------------------

#include <vector>
#include <set>
#include <map>

#include <OS.h>

// ---------------------------------------------------------------------------
// Buckets
// Buckets are the profile "slots" passed to the profiler
// They consist of an address and a count
// ---------------------------------------------------------------------------

class Bucket {
public:
	Bucket(unsigned long address, int32 count)
	{ 
		fAddress = address; 
		fHitCount = count; 
	}

	Bucket(int32 address)
	{ 
		Bucket(address, 0); 
	}

	void AddToCount(const Bucket& other) const
	{
		// I need to declare AddToCount const because all the set iterators are constant,
		// but I also need to bump the count when the symbol was already found in the set
		// I can safely add to this constant CountedSymbol since this doesn't change
		// equality or sort order.
		Bucket* mutableThis = const_cast<Bucket*>(this);		
		mutableThis->fHitCount += other.fHitCount;
	}
		
public:
	unsigned long	fAddress;
	int32			fHitCount;
};


inline bool operator<(const Bucket& left, const Bucket& right)
{
	return left.fAddress < right.fAddress;
}

inline bool operator==(const Bucket& left, const Bucket& right)
{
	return left.fAddress == right.fAddress;
}

inline bool operator>(const Bucket& left, const Bucket& right)
{
	return left.fAddress > right.fAddress;
}

// ---------------------------------------------------------------------------
// Image handling
// ---------------------------------------------------------------------------

class ImageList {
public:
	bool IsLoaded(image_id id) const;
	bool AddImage(image_id id);
	bool RemoveImage(image_id id);

private:
	vector<image_id> fImages;
	typedef vector<image_id> ImageVector;
};

// ---------------------------------------------------------------------------
// Symbol Handling
// ---------------------------------------------------------------------------

typedef pair<unsigned long, const char*> SymbolPair;

class SymbolTable {
public:
					SymbolTable();
					~SymbolTable();
	bool			LoadImageSymbols(image_id id, const char* name);
	bool			LoadTeamSymbols(team_id teamID);
	const char* 	GetSymbolName(unsigned long addr);
	int32			Count() const 				{ return fMap.size(); }
	void			FillBuckets(Bucket* slots);

private:
	void AddSymbol(unsigned long location, const char* name);

#ifdef __ELF__
	status_t LoadSymbolsFromELFFile(const char* path, ulong codeaddr, ulong dataaddr);	
#else
	status_t LoadSymbolsFromPEFFile(const char* path, ulong codeaddr, ulong dataaddr);	
#endif

	typedef map<unsigned long, const char*> SymbolMap;
	SymbolMap	fMap;
	ImageList	fLoadedImages;
};

// ---------------------------------------------------------------------------
// Thread information that we need to keep track of for the life of each thread
// ---------------------------------------------------------------------------

class ThreadInfo {
public:
							ThreadInfo(thread_id id);

	void 					AddToTopCounts(const Bucket& aBucket);
	const char*				ThreadName() const { return fName; }

	// Thread name can be exactly B_OS_NAME_LENGTH, so we allow B_OS_NAME_LENGTH+1 for null
	char					fName[B_OS_NAME_LENGTH+1];
	set<Bucket>				fTopCount;
};

// ---------------------------------------------------------------------------
// Thread Handling
// ---------------------------------------------------------------------------

typedef pair<thread_id, ThreadInfo*> ThreadPair;
typedef void (*ThreadFunction)(const ThreadPair&);

class ThreadList {
public:
				ThreadList();
				~ThreadList();
	bool 		AddThread(thread_id id);
	bool 		RemoveThread(thread_id id);
	const char*	GetThreadName(thread_id id) const;
	bool 		IsRunning(thread_id id) const;
	void 		ForEach(ThreadFunction func);
	ThreadInfo*	GetThreadInfo(thread_id id);
	
private:
	map<thread_id, ThreadInfo*> fThreads;
	typedef map<thread_id, ThreadInfo*> ThreadMap;
};
