// ===========================================================================
//	Cache.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Cache.h"
#include "UResource.h"
#include "NPApp.h"
#include "Store.h"
#include "MessageWindow.h"

#include <time.h>
#include <stdlib.h>
#include <TextView.h>
#include <Window.h>
#include <Screen.h>
#include <Directory.h>
#include <NodeInfo.h>
#include <Path.h>
#include <FindDirectory.h>
#include <stdio.h>
#include <Autolock.h>

#define CACHETODISK
#define WRITELOG

extern const char *kDefaultCacheLocation;
extern const char *kCacheIndexFileName;
extern const char *kCacheFileName;
extern const char *kCookieFileName;
extern const char *kApplicationName;
extern const char *kApplicationSig;

static int32 cacheCleanerCount = 0;
BDirectory	sCacheFolder;

static bool initialized = false;


Store* UResourceCache::ReadCacheData(const char *name)
{
#ifdef DONTCACHE
	return NULL;
#endif

	// If another thread is performing initialization, hang out until it's done.
	// This is a pretty cheesy hack, but it works.
	while (!initialized)
		snooze(50000);

	BEntry entry;
	status_t err = sCacheFolder.FindEntry(name,&entry);
	if (err == B_NO_ERROR) {
		Store* store = new BufferedStore(new BFileStore(&entry));	// Create read only store
		store->Open(true);
		if (store->GetStatus() == kError) {
			delete store;
			return NULL;
		}
		return store;
	}

	pprintBig("ReadCacheData: Can't find '%s'",name);
	return NULL;
}

entry_ref UResourceCache::GetFileFromCache(const char *name)
{
	BEntry entry;
	sCacheFolder.FindEntry(name, &entry);
	entry_ref ref;
	entry.GetRef(&ref);
	return ref;
}

//	Write a file to the cache folder

void UResourceCache::WriteCacheData(const char *name, const char *type, Store *store)
{
#ifdef DONTCACHE
	return;
#endif

	if (store == NULL)
		return;
		
	status_t err;
	
	BEntry entry;
	err = sCacheFolder.FindEntry(name,&entry);	// May already exist?
	if (err == B_NO_ERROR)
		entry.Remove();

	long count = store->GetLength();
	void *data = store->GetData(0,count);

	if (data) {
		BFile file;
		err = sCacheFolder.CreateFile(name,&file);
		if (err != B_NO_ERROR) {
			pprint("WriteCacheData: Failed to create '%s' (0x%x)",name,err);
			return;
		}
	
//		NP_ASSERT(count > 0);
//		NP_ASSERT(data);
		
		err = file.Write(data,count);
		if (err < B_NO_ERROR || err != count)
			pprint("/*************************************************\nWriteCacheData: Failed to write %d bytes of data at 0x%x to '%s' (0x%x)\n*************************************************/",count, data, name,err);

		err = sCacheFolder.FindEntry(name,&entry);	// May already exist?
		if (err == B_NO_ERROR) {
			BNode node(&entry);
			BNodeInfo info(&node);
			info.SetType(type);
			info.SetPreferredApp(kApplicationSig);
		}

	}
	store->ReleaseData(0,count);
}

void UResourceCache::CopyFromCache(const char *cacheName, const char *destPath)
{
	BEntry srcEntry, dstEntry(destPath);
	if (sCacheFolder.FindEntry(cacheName,&srcEntry) == B_NO_ERROR)
		CopyFile(&srcEntry, &dstEntry);	
}


void
RemoveCacheFile(
	const char	*name)
{
	BEntry entry;
	status_t err = sCacheFolder.FindEntry(name,&entry);	
	if (err == B_NO_ERROR)
		entry.Remove();
}

//===========================================================================
// Extract the next string, return number of chars used or zero if no complete string was formed

long NextStr(char *d, long count, char *dst)
{
	short c;
	short i = 0;
	long  oldCount = count;

	dst[0] = 0;
	while (count--) {
		c = (dst[i++] = *d++);
		if ((c == 0x0D) || (c == 0x0A)) {
			dst[i-1] = 0;					// Got a complete line, strip the end of line char
			if (count) {					// Must be sure about last char
				if (((*d == 0x0D) || (*d == 0x0A)) && *d != c) {	// Don't strip 0x0A,0x0A or 0x0D,0x0D
					count--;
					d++;					// But strip CRLF
				}
				return oldCount - count;
			}
		}
	}
	return 0;
}


//===========================================================================
//	Version of a hash table that can sort based on the Cache Score

int compCacheScore(const void* a,const void* b)
{
	float aa = (*((UResourceImp**)a))->GetCacheScore();
	float bb = (*((UResourceImp**)b))->GetCacheScore();
	if (aa < bb)
		return -1;
	else if (aa > bb)
		return 1;
	return 0;
}

void ImpTable::BuildIndex()		// Sort index after its built
{
	HashTable::BuildIndex();
	if (mValidIndex == false)
		return;
		
	qsort(mIndex,mCount,4,compCacheScore);
}

//===========================================================================
//	The resource cache is based on UResourceImp objects, not UResource
//	The UResourceImp may be a reference to a disk file that has not yet been loaded

ImpTable	*UResourceCache::mImps =  new ImpTable();
bool		UResourceCache::mOpen = false;
uint64		UResourceCache::mCacheSize = 0;
uint64		UResourceCache::mMaxCacheSize = 5 * 1024 * 1024;
int			UResourceCache::mCacheOption = 1;
BString		UResourceCache::mCacheLocation;
static TLocker sCacheLock("Cache Lock");

//===========================================================================


int32 UResourceCache::Init(void *)
{
	if (!initialized) {
#ifndef DONTCACHE	
		if(create_directory(UResourceCache::GetCacheLocation(), 0755) != B_OK){
			printf("Couldn't Create Cache \"%s\"\n", UResourceCache::GetCacheLocation());
			return 0;
		}
	
		if (sCacheFolder.SetTo(UResourceCache::GetCacheLocation()) != B_OK)
			return 0;
			
		sCacheFolder.SetTo(UResourceCache::GetCacheLocation());	
		initialized = true;
		Open();

		// Once in a while, go through and remove items in the cache folder
		// that don't belong.
		int32 launchCount = gPreferences.FindInt32("LaunchCount");
		if ((launchCount % 20) == 0)
			UResourceCache::CleanCache(true);
		else
			UResourceCache::CleanCache(false);
#endif
	}
	return 0;
}

void UResourceCache::Lock()
{
	sCacheLock.Lock();
}

void UResourceCache::Unlock()
{
	sCacheLock.Unlock();
}

//	Cache resourceImp's by linking them together

void UResourceCache::Add(UResourceImp *resource)
{
	if (sCacheLock.LockWithTimeout(TEN_SECONDS) != B_OK)
		return;

	UResourceImp *imp = Lookup(resource->GetURL());
	if (imp) {
		pprint("Deleting stale cache resource.");
		Delete(imp);
	}
	
//	NP_ASSERT(resource);
//	NP_ASSERT(imp);
	resource->SetUpToDate(true);
	if (resource->GetUsageCount() == 0)
		resource->SetUsageCount(1);
	if (resource->GetLastVisited() == 0)
		resource->SetLastVisited(time(NULL));
	resource->RecalcCacheScore();
	mImps->Add(resource);
	
	if (resource->GetCacheOnDisk()) {
		mCacheSize += resource->GetContentLength();
		
		if (mCacheSize > mMaxCacheSize)
			CleanCache();
	}

	Unlock();
}

// If a resource is in the cache, we need to be careful about changing its
// URL, since the cache is a hash table based on the URL.  If it's cached, remove
// its cache entry under the old name and re-add it under the new name.
void UResourceCache::ChangeURL(UResourceImp *imp, const char *newURL)
{
	Lock();

	mImps->Remove(imp);

	imp->SetURL(newURL);

#ifdef DEBUGMENU
	if (Lookup(newURL))
		pprint("URLChanged: New URL %s already in cache.", newURL);
#endif

	mImps->Add(imp);

	Unlock();
}

void UResourceCache::DontCache(UResourceImp *imp)
{
	Lock();
	mImps->Remove(imp);
	Unlock();
}

//	Delete a resource if it is dead

bool UResourceCache::Delete(UResourceImp *imp)
{
	bool result = false;
	Lock();

	char *cacheName = (imp->GetCacheName() != NULL) ? strdup(imp->GetCacheName()) : 
															NULL;

	if (imp->GetContentLength() > mCacheSize)
		mCacheSize = 0;
	else
		mCacheSize -= imp->GetContentLength();

	if (imp->GetRefCount() == 0) {
		result = true;
		if (!mImps->Delete(imp))
			delete imp;

		if (cacheName != NULL) {
			RemoveCacheFile(cacheName);
			free(cacheName);
		}
	}

	Unlock();
	
	return result;
}

void UResourceCache::CleanCache(bool removeUnusedFiles)
{
	if (atomic_add(&cacheCleanerCount, 1) == 0) {
		bool *args = new bool;
		if (removeUnusedFiles)
			*args = true;
		else
			*args = false;
		thread_id tid = spawn_thread(UResourceCache::CleanCacheEntry, "Cache cleaner", B_LOW_PRIORITY + 2, args);
		resume_thread(tid);
		NetPositive::AddThread(tid);
	} else
		atomic_add(&cacheCleanerCount, -1);
}


int32 UResourceCache::CleanCacheEntry(void *args)
{
	bool removeUnused = *(bool *)args;
	delete (bool*)args;
	if (removeUnused) {
		// Try to remove any files in the cache which aren't referenced in the
		// cache log (probably due to a program crash).  This code isn't thread-safe
		// with respect to the cache table, but it shouldn't cause any harm.
		BDirectory cacheFolder(sCacheFolder);
		cacheFolder.Rewind();
		BEntry entry;
		while (cacheFolder.GetNextEntry(&entry) == B_OK) {
			char name[B_FILE_NAME_LENGTH];
			entry.GetName(name);
			if (strcmp(name, kCacheIndexFileName) == 0 ||
				strcmp(name, kCookieFileName) == 0)
					continue;
			int index = 0;
			UResourceImp *imp = 0;
			bool foundIt = false;
			Lock();
			do {
				imp = (UResourceImp *)mImps->Get(index);
				if (imp && imp->GetCacheName() && strcmp(name, imp->GetCacheName()) == 0) {
					foundIt = true;
					break;
				}
				index++;
			} while (imp);
			Unlock();
			// If somebody else is waiting for the resource cache, give them a shot.
			if (sCacheLock.CountLockRequests() > 0)
				snooze(50000);
			if (!foundIt) {
				RemoveCacheFile(name);
			}
		}
	} else {
		uint32 index = 0;
		UResourceImp *previousImp = 0;
		while (mCacheSize > mMaxCacheSize) {
			{
				// This isn't perfectly thread-savvy, but it won't hurt us any.  If mImps
				// changes from under us, we will deal with it fairly gracefully.
				BAutolock autolock(sCacheLock);
				UResourceImp *imp = (UResourceImp *)mImps->Get(index);
				
				if (imp == previousImp)
					// Try to prevent an infinite loop, even though we should never get here.
					break;
				previousImp = imp;
				if (imp) {
					if (!Delete(imp))
						index++;
				} else {
					break;
				}
			}
			// If somebody else is waiting for the resource cache, give them a shot.
			if (sCacheLock.CountLockRequests() > 0)
				snooze(50000);
		}
	}	
	atomic_add(&cacheCleanerCount, -1);
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}


void UResourceCache::EraseCache()
{
	if (atomic_add(&cacheCleanerCount, 1) == 0) {
		thread_id tid = spawn_thread(UResourceCache::EraseCacheEntry, "Cache eraser", B_LOW_PRIORITY + 2, 0);
		resume_thread(tid);
		NetPositive::AddThread(tid);
	} else
		atomic_add(&cacheCleanerCount, -1);
}


int32 UResourceCache::EraseCacheEntry(void *)
{
	uint32 index = 0;
	UResourceImp *imp;
	UResourceImp *oldImp = 0;
	do {
		Lock();
		imp = (UResourceImp *)mImps->Get(index);
		if (imp == oldImp)
			break;
		oldImp = imp;
		if (imp && !Delete(imp))
			index++;
		Unlock();
		// If somebody else is waiting for the resource cache, give them a shot.
		if (sCacheLock.CountLockRequests() > 0)
			snooze(50000);
	} while (imp);
		
	atomic_add(&cacheCleanerCount, -1);
	NetPositive::RemoveThread(find_thread(NULL));
	return 0;
}

const char *UResourceCache::GetCacheLocation()
{
	if(mCacheLocation.Length() == 0){
		BString prefCache(gPreferences.FindString("CacheDirectory"));
		if(prefCache.Length() == 0){
			BPath location;
			find_directory(B_USER_SETTINGS_DIRECTORY, &location, true);
			mCacheLocation << location.Path() << kDefaultCacheLocation;
		}
		else 
			mCacheLocation.SetTo(prefCache);
	}
	return mCacheLocation.String();
}

void UResourceCache::SetCacheLocation(const char *loc)
{
	if (loc && *loc)
		mCacheLocation.SetTo(loc);
	else
		mCacheLocation.SetTo("");
	
}

void UResourceCache::AddToCacheSize(long size)
{
	mCacheSize += size;
}

//	Purge a resource from the memory cache in response to a reload

void UResourceCache::Purge(const char *url)
{
	UResourceImp* imp = Lookup(url);
	if (imp) {
		imp->MarkForDeath();	//
		imp->RefCount(0);		// Will delete the resource from the cache
	}
}

//	Commit to cache when load worked
		
void UResourceCache::CacheOnDisk(UResourceImp *imp)
{
	if (!imp) {
		pprint("UResourceCache::CacheOnDisk: Invalid UResourceImp!");
		return;
	}
	
	if (imp->IsDead())
		return;

	char cacheName[256];
	NewCacheName(imp,cacheName);
	imp->SetCacheName(cacheName);
	
	pprint("Caching '%s' as '%s'",imp->GetURL(),cacheName);
#ifdef CACHETODISK
	WriteCacheData(cacheName,imp->GetContentType(),imp->GetStore());
#endif
}

//	Hilight link if we have visited

bool UResourceCache::HasBeenVisited(const char *url)
{
	return Lookup(url) != NULL;
}

//	Lookup UResourceImp in cache

UResourceImp* UResourceCache::Lookup(const char *url)
{
//	NP_ASSERT(url);
	Lock();

	UResourceImp* limp = (UResourceImp*)mImps->Lookup(url,strlen(url));

	Unlock();

	return limp;
}

//	Return a new resource if url is in cache

UResourceImp *UResourceCache::Cached(
	const char	*url,
	bool		force)
{
	if (sCacheLock.LockWithTimeout(TEN_SECONDS) != B_OK)
		return NULL;

	UResourceImp* imp = Lookup(url);
	if (imp == NULL) {
		pprint("No cache entry for %s", url);
		Unlock();
		return NULL;
	}
	
	if (imp->IsDead()) {
		Unlock();
		return NULL;
	}

	if (!force) {
		if (!imp->IsUpToDate() || (imp->GetExpires() != -1 && imp->GetExpires() <= time(NULL))) {
			pprint ("Cache for %s was stale", url);
			Unlock();
			return (NULL);
		}
	}
	
	StoreStatus status = imp->GetStatus();
	if (status == kTimeout || status == kAbort || status == kError) {
		pprint("Cache for %s was incomplete", url);
		Unlock();
		return (NULL);
	}

//	If resource is in disk cache and has not been loaded yet,
//	create a store and load it from disk

	const char *cacheName = imp->GetCacheName();
	if (cacheName && imp->GetStore() == NULL) {
		Store *store = ReadCacheData(cacheName);	// Load data in a store, attach it to imp
		
		if (store == NULL) {								// Item was missing from cache or failed to load
			pprint("'%s' was missing from cache",cacheName);
			imp->MarkForDeath();							// Kill this resource when nobody is using it
			imp->RefCount(0);								// Will delete immediately if no one is using it
			Unlock();
			return NULL;
		}
		imp->SetStore(store);
	}
	
		
//	UResource *r = new UResource(imp);	// Create a new resource from this imp

	imp->RefCount(1);

	pprint("Fetching %s from cache entry %s.  Imp is 0x%x", url, cacheName, (unsigned int)imp);
	imp->IncrementUsageCount();
	imp->SetLastVisited(time(0));
	imp->RecalcCacheScore();

	Unlock();

//	return r;		// Create a new resource
	return imp;
}

//	Load the log file from disk

void UResourceCache::Open()
{
	Store *store = NULL;
	store = ReadCacheData(kCacheIndexFileName);
	if (store == NULL)
		return;
		
	Lock();

	char* url = (char*)malloc(8192);
//	NP_ASSERT(url);
	char* str = (char*)malloc(8192);
//	NP_ASSERT(str);

	long count = store->GetLength();
	char *d = (char *)store->GetData(0,count);
	long now = time(NULL);

	while (count) {
		long lastVisited,lastMod,expires;
		char cacheName[256];
		char type[256];
		long length;
		long usageCount = 1;
		
		int i = NextStr(d,count,str);
		if (!i) break;
		
		if (sscanf(str,"%ld %ld %ld %s %s %s %ld %ld",&lastVisited,&lastMod,&expires,cacheName,url,type,&length,&usageCount) >= 7) {

			UResourceImp *imp = new UResourceImp(url,NULL);
			imp->SetDate(0);
			imp->SetLastVisited(lastVisited);
			imp->SetLastModified(lastMod);
			imp->SetExpires(expires); // The Imp will bounds check the expiration date against its own heuristics.
			imp->SetCacheName(cacheName);
			imp->SetContentType(type);
			imp->SetContentLength(length);
			imp->SetUsageCount(usageCount);
			
			mCacheSize += length;

			if (expires > now && expires != -1 && imp->GetExpires() != -1) {
				imp->SetUpToDate(true);
			} else {
				imp->SetUpToDate(false);
			}
			mImps->Add(imp);
		}
		d += i;
		count -= i;
	}
	
	int i = 0; 
	while (UResourceImp *imp = (UResourceImp *)mImps->Get(i)) {
		imp->RecalcCacheScore();
		i++;
	}
	
	free(url);
	free(str);
	delete store;
	
	mOpen = true;
	Unlock();

	return;
}

//	Create a new name for this imp
//	Might add .jpg or .mov or something useful to the name.....

void UResourceCache::NewCacheName(UResourceImp*, char* cacheName)
{
	cacheName[0] = 0;
	sprintf(cacheName,kCacheFileName,real_time_clock_usecs());
}

//	Log all UResourceImp's

void UResourceCache::Close()
{
	BucketStore* store = new BucketStore;
	UResourceImp *imp;
	const char *str;
	
	str = "--Cache Log--\n";
	store->Write(str,strlen(str));

#ifdef WRITELOG
	Lock();
	int i = 0;
	char *str2 = 0;
	int strlen2 = 0;
	while ((bool)(imp = (UResourceImp *)mImps->Get(i))) {
		const char *cacheName = imp->GetCacheName();		
		if (cacheName) {							// If GetCacheName()==NULL, don't add imp to log
			int newlen = 128 + strlen(cacheName);
			const char *str = imp->GetURL();
			if (str) newlen += strlen(str);
			str = imp->GetContentType();
			if (str) newlen += strlen(str);

			if (newlen > strlen2 || str2 == 0) {
				if (str2)
					free(str2);
				str2 = (char *)malloc(newlen);
				strlen2 = newlen;
			}
			sprintf(str2,"%ld %ld %ld %s %s %s %ld %ld\n",
				imp->GetLastVisited(),				// Time last visited
				imp->GetLastModified(),
				imp->GetExpires(),
				cacheName,
				imp->GetURL(),
				imp->GetContentType(),
				imp->GetContentLength(),
				imp->GetUsageCount());
			store->Write(str2,strlen(str2));			// Cache log entry holds all http fields
		}
		i++;
	}
	if (str2)
		free(str2);
	Unlock();
#endif

	str = "--Cache Log--\n";
	store->Write(str,strlen(str));
	
	WriteCacheData(kCacheIndexFileName,"text/plain",store);
	delete store;
	
	mOpen = false;
}

