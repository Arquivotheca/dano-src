// ===========================================================================
//	Cache.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __CACHE__
#define __CACHE__

#include "HashTable.h"
#include <SupportDefs.h>
#include <String.h>
struct entry_ref;

//===========================================================================
//	Version of a hash table that can sort based on LastVisited

class ImpTable : public HashTable {
public:
virtual	void	BuildIndex();		// Sort index after its built
};

//===========================================================================
//	List of memory resident resource imps

class UResourceImp;
class Store;

class UResourceCache {
public:
	static	int32			Init(void *args);
	static	void			Lock();
	static	void			Unlock();
	
	static	void			Open();		// Read list of UResources Cached on disk
	static	void			Close();

	static	UResourceImp*	Lookup(const char *url);
	static	UResourceImp*	Cached(const char *url, bool force);
	static	bool			HasBeenVisited(const char *url);

	static	void			Add(UResourceImp *resource);
	static	void			CacheOnDisk(UResourceImp *imp);
	static	bool			Delete(UResourceImp *imp);
	static	void			Purge(const char *url);
	
	static	void			CleanCache(bool removeUnusedFiles = false);
	static	void			AddToCacheSize(long size);
	
	static	uint64			GetMaxCacheSize() {return mMaxCacheSize;}
	static	void			SetMaxCacheSize(uint64 size) {mMaxCacheSize = size;}
	
	static  int				GetCacheOption() {return mCacheOption;}
	static	void			SetCacheOption(int option) {mCacheOption = option;}
	
	static	const char *	GetCacheLocation();
	static	void			SetCacheLocation(const char *loc);
	static	void			EraseCache();
	
	static Store*			ReadCacheData(const char *name);
	static void				WriteCacheData(const char *name, const char *type, Store *store);
	static void				CopyFromCache(const char *cacheName, const char *destPath);
	static entry_ref		GetFileFromCache(const char *name);
	static void				ChangeURL(UResourceImp *imp, const char *newURL);
	static void				DontCache(UResourceImp *imp);
	
protected:
	static	void			NewCacheName(UResourceImp* imp,char *cacheName);
	static	int32			CleanCacheEntry(void *args);
	static	int32			EraseCacheEntry(void *args);
	
	static	ImpTable		*mImps;		// List of resource implementations
	static	bool			mOpen;
	static	uint64			mCacheSize;
	static	uint64			mMaxCacheSize;
	static	int				mCacheOption;
	static	BString			mCacheLocation;
};

//===========================================================================

#endif
