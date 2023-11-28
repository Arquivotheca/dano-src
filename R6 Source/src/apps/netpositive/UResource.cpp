// ===========================================================================
//	UResource.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995,1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "UResource.h"
#include "Cache.h"
#include "URL.h"
#include "Protocols.h"
#include "MIMEType.h"
#include "NPApp.h"
#include "Strings.h"
#include "Store.h"
#include "MessageWindow.h"
#include "NetPositiveStreamIO.h"
#ifdef PLUGINS
#include "PluginSupport.h"
#endif

#include <malloc.h>
#include <Directory.h>
#include <Path.h>
#include <time.h>
#include <stdio.h>
#include <StorageKit.h>

// ===========================================================================
//	Read a resource in from the net

UResourceImp *ReadFromNet(
	ConnectionManager *mgr,
	URLParser&	parser, 
	BString* 	formData,
	long,
	const char	*downloadPath,
	BMessenger *listener,
	const char	*referrer,
	bool		forceDownload,
	uint32		rangeBegin,
	bool		useStreamIO,
	bool		dontReuse)
{
	//pprintBig("ReadFromNet: Read '%s' from net",(const char*)url);
	
	BString url;
	parser.WriteURL(url);
	if (gEZDebug){
		printf("Opening URL %s\n", url.String());
		fflush(stdout);
	}
	
	UResourceImp	*imp = new UResourceImp(url.String(), NULL, listener, downloadPath, useStreamIO);
//	UResourceImp	*imp = new BeUResourceImp(url, downloadPath, listener);
//	UResource		*resource = new UResource(imp);
	imp->RefCount(1);

//	Guess a correct scheme for this url, normally know explicitly

	URLScheme scheme = parser.GuessScheme();
	
//	Create a new protocol looper

	switch (scheme) {
		case kFTP:
		case kFILE:
			mgr->GetFTP(imp, url, downloadPath, forceDownload, rangeBegin);
			imp->SetCacheOnDisk(false);			// Cache FTP stuff for a while....
			break;
		case kHTTP:
#ifndef NOSSL
		case kHTTPS:
#endif
		{
			BString referrerString;
			if (referrer && *referrer)
				referrerString = referrer;
			else
				referrerString = mgr->GetReferrer();
			mgr->GetHTTP(imp, url, formData, &referrerString, downloadPath, scheme == kHTTPS, NULL, 0, false, forceDownload, rangeBegin, dontReuse);
// We don't ordinarily want to cache form submissions, but there are exceptions:  back/forward and save to disk
// depend on them being cached to work the way you'd expect.  So, we'll set everything to be cached to disk,
// but form submissions will be set to expired.  (The operations mentioned before force cache fetches even if
// the items are expired).  For CGI submissions, the same rule applies, but it's handled earlier by the resource
// imp when it sees a question mark in the URL.
			imp->SetCacheOnDisk(true);
			if (formData != NULL)
				imp->SetExpires(time(NULL));
			break;
		}
		default:
			pprint("Unknown scheme in ReadFromNet");
//			delete (resource);
			imp->RefCount(-1);
			return NULL;
	}
	if (downloadPath != NULL) {
		imp->SetCacheOnDisk(false);
	}
	
//	return (resource);
	return imp;
}

//	Guess document type base on filename and the first little bit of data

void DetermineFileType(const char *filename, ulong, const char *data, int dataCount, char *typeStr)
{
pprint("DetermineFileType %s, data count %d", filename, dataCount);
	MimeType* m = MimeType::MatchMime(filename,data,dataCount);
	if (m)
		strcpy(typeStr,m->GetMimeType());
	else
		*typeStr = 0;
}

// Copy a file.

void CopyFile(BEntry *srcEntry, BEntry *targetEntry)
{
	BFileStore src(srcEntry);
	BFileStore target(targetEntry);
	
	src.Open(true);
	target.Open(false);
	
	void *data = malloc(65536);
	long lastLength;
	if (data) {
		do {
			lastLength = src.Read(data, 65536);
			if (lastLength)
				target.Write(data, lastLength);
		} while (lastLength > 0);

		free(data);
	}
	
	char fileType[B_MIME_TYPE_LENGTH];
	src.GetType(fileType);	
	target.SetType(fileType);
	
	src.Close();
	target.Close();
}

UResourceImp *CopyResourceToStream(UResourceImp *src)
{
	UResourceImp *dest = new UResourceImp("",NULL, NULL, NULL, true);
	dest->RefCount(1);
	if (!dest)
		return NULL;
		
	uint32 pos = 0;
	while (pos < src->GetLength()) {
		long size = MIN(1024, src->GetLength() - pos);
		void *ptr = src->GetData(pos, size);
		if (ptr)
			dest->Write(ptr, size);
		pos += size;
		src->ReleaseData(ptr, size);
	}
	
	dest->Flush(kComplete);
	
	return dest;
}

//	Create a new resource from a file on an existing volume

UResourceImp *FileResource(const char *url, entry_ref* ref, const char *downloadPath, BMessenger *listener, bool useStreamIO)
{
	BEntry entry(ref, true);
	BFile file(&entry,O_RDWR);
	
	off_t size;
	entry.GetSize(&size);
	if (size <= 0)			// No data here, don't bother
		return NULL;
		
//	Identify file type

	char filename[256];
	ulong fileType = 0;
	
	entry.GetName(filename);
	

//  If we have a downloadPath, then the user wants to save this URL to a separate disk
//  file.  Create a BEntry for the target file and perform the copy.
	if (downloadPath) {
		BEntry targetEntry(downloadPath);
		CopyFile(&entry, &targetEntry);
	}
	
//	Create a store that uses the data in the disk file

	Store* store = new BufferedStore(new BFileStore(&entry));	// Create read only store
	store->Open(true);
	UResourceImp* imp = new UResourceImp(url,store, listener, NULL, useStreamIO);
	
//	Determine and set mime type

	char typeStr[256];
	strcpy(typeStr,"application/octet-stream");
	
	long dataCount = MIN(256,store->GetLength());
	if (dataCount) {
		char* data = (char*)store->GetData(0,dataCount);					// First 256 bytes of file
		if (data) {
			BString fn(filename);
			fn.ToLower();
			DetermineFileType(fn.String(),fileType,data,dataCount,typeStr);	// Guess what type of data
			store->ReleaseData(data,dataCount);
		}
	}

	imp->SetContentType(typeStr);
	imp->SetContentLength(store->GetLength());
	imp->NotifyListeners(msg_ResourceChanged);
	imp->NotifyListeners(msg_ResourceFlushed);
//	return new UResource(imp);
	imp->RefCount(1);
	if (useStreamIO) {
		UResourceImp *copy = CopyResourceToStream(imp);
		imp->RefCount(-1);
		return copy;
	}
	return imp;
}

//	Create and HTML page from a directory

UResourceImp*	DirectoryResource(const char* url, BDirectory& directory, const char *downloadPath, BMessenger *listener, bool useStreamIO)
{
	char 		name[B_FILE_NAME_LENGTH + 1];

	UResourceImp* imp;
	imp = new UResourceImp(url, NULL, listener, downloadPath, useStreamIO);
//	imp = new BeUResourceImp(url, downloadPath, listener);

	FileSystemHTML fs(imp);

	fs.WriteHeader();

//	Add all the files in this directory

	BEntry entry;
	directory.Rewind();
	while (directory.GetNextEntry(&entry) == B_NO_ERROR) {
		entry.GetName(name);
		bool isFile = false;
		if (entry.IsSymLink()) {
			entry_ref ref;
			entry.GetRef(&ref);
			BEntry entry2(&ref, true);
			if (entry2.IsFile())
				isFile = true;
		} else if (entry.IsFile())
			isFile = true;
		else
			isFile = false;
			
		if (isFile) {
			off_t size;
			entry.GetSize(&size);
			fs.AddFile('-',size,0,name);
		} else
			fs.AddFile('d',0,0,name);
	}
		
//	Write file list
	fs.WriteTrailer();

	imp->Flush(kComplete);
	
//	return new UResource(imp);
	imp->RefCount(1);
	return imp;
}


//	Read a resource in from disk, either a file or a directory

UResourceImp *ReadFromDisk(URLParser& parser, long, const char *downloadPath, BMessenger *listener, bool useStreamIO)
{
	UResourceImp* r = NULL;
	BString url;
	
	char path[2048];				// I hate this static allocation shit
	
	if (parser.Path() == NULL)			// Default path is "/"
		strcpy(path,"/");
	else
		CleanName(parser.Path(),path);	// Strip %20 escape codes from url
		
	const char *query = parser.Query();
	if (query && *query) {
		strcat(path,"?");
		strcat(path,query);
	}
		
	entry_ref ref;
	if (get_ref_for_path(path,&ref) == B_NO_ERROR) {
		BEntry entry(&ref, true);
		if (entry.IsFile() || entry.IsSymLink()) {	// Could be a file or a directory
			parser.WriteURL(url);
			r = FileResource(url.String(),&ref, downloadPath, listener, useStreamIO);				// Its a file
		}
			
		else if (entry.IsDirectory()) {
			BPath p;
			entry.GetPath(&p);
			
			strcpy(path,p.Path());
			if (gPreferences.FindBool("LimitDirectoryBrowsing")) {
				BPath userDirPath;
				find_directory(B_USER_DIRECTORY, &userDirPath);
				if (strncasecmp(path, userDirPath.Path(), strlen(userDirPath.Path())) != 0)
					return NULL;
			}
			char* u = path;
			int i = strlen(path);
			if (u[i-1] != '/') {
				u[i] = '/';
				u[i+1] = 0;							// Make sure directories end in a slash
			}
				
			url = "file:";
			url += path;
			BDirectory dir(&entry);
			r = DirectoryResource(url.String(),dir, downloadPath, listener, useStreamIO);
		}
	}

	if (r == NULL)
		pprint("ReadFromDisk: Can't read '%s' from disk",url.String());
	return r;
}

//	Return a resource that is the startup page

UResourceImp *ReadLocal(URLParser& parser, long, const char *downloadPath, BMessenger *listener, bool useStreamIO)
{
	BString resourceName;
	parser.WriteURL(resourceName);
	
	// Let's see if there's a substitute resource available.  First, let's check the resource name
	// against the list of resources where substitutes are not permitted.  If it's not on that list,
	// we'll look for B_USER_SETTINGS_DIRECTORY/NetPositive/Resources/<resource_name>, and if we
	// find it, we'll use it.  If we don't find it, we'll use the internal one.
	const char **disallowList = DisallowSubstitutionList();
	bool disallow = false;
	while (disallowList && *disallowList && !disallow) {
		if (resourceName == *disallowList)
			disallow = true;
		disallowList++;
	}
	if (!disallow) {
		BPath path;
		find_directory(B_USER_SETTINGS_DIRECTORY, &path);
		BString resPath("file:");
		resPath << path.Path() << kResourceFolderLocation << parser.Path();
		URLParser resParser;
		resParser.SetURL(resPath.String());
		UResourceImp *imp = ReadFromDisk(resParser, 0, downloadPath, listener, useStreamIO);
		if (imp)
			return imp;
	}
	
	const char* path = resourceName.String() + strlen(kInternalURLPrefix);
	
	size_t	size = 0;
	char	*data = (char *)GetNamedResource(path, size);
	if (data == NULL) {
		pprint("GetNamedResource for %s failed",path);
		return NULL;
	}
	
	if (downloadPath) {
		BEntry entry(downloadPath);
		BFileStore target(&entry);
		target.Open(false);
		target.Write(data, size);
		target.Close();
	}

	char typeStr[256];
	DetermineFileType(resourceName.String(),0,data,size,typeStr);		// Find out what kind of data this is
	
	BString typeCStr(typeStr);
	UResourceImp* r = NewResourceFromData(data,size,resourceName,typeCStr, listener, useStreamIO);
	free(data);
	return r;
}


//===========================================================================
//	Get a universal resource
//	'path' is the current path for resolving relative urls

UResourceImp *GetUResource(ConnectionManager *mgr, const BString& url, long docRef, BString& errorMessage, 
						bool forceCache, const char *downloadPath, BMessenger *listener, const char *referrer,
						bool forceDownload, uint32 rangeBegin, bool useStreamIO, bool dontReuse)
{
	UResourceImp *resource = NULL;
	errorMessage = "";

	//if (downloadPath == NULL) {
#ifdef DEBUGMENU
	// The Yahoo! Random Link will be in the resource cache until we get the redirection back
	// from the server.  If you run the random link spamming command in a number of windows
	// simultaneously, different windows will occasionally get the cached resource for this
	// link instead of a fresh resource and the windows will all display the same page.  Since
	// this isn't a problem for normal browsing, make a special case of it for debugging.
	if (url != kDebugYahooRandomLink)
#endif

	resource = UResourceCache::Cached(url.String(), forceCache);
	if (resource != NULL) {
		if (useStreamIO) {
			UResourceImp *copy = CopyResourceToStream(resource);
			resource->RefCount(-1);
			return copy;
		}
		if (downloadPath) {
			const char *name = resource->GetCacheName();
			pprint("Copying from cache file %s", name);
			if (!name) {
				pprint("Oops, no cache file.  Trying to cache to disk first");
				UResourceCache::CacheOnDisk(resource);
				name = resource->GetCacheName();
				pprint("New name is %s", name);
				if (!name)
					resource = NULL;
			}
			if (resource) 
				UResourceCache::CopyFromCache(resource->GetCacheName(), downloadPath);
		}
		if (resource) {
			if (listener)
				resource->AddListener(listener);
			resource->NotifyListeners(msg_ResourceChanged);
			resource->NotifyListeners(msg_ResourceFlushed);
			return (resource);
	
		}
	}

//	Parse URL, decide how we are going to get the data

 	URLParser parser;
 	parser.SetURL(url.String());
 	
//	Guess a correct scheme for this url, normally know explicitly

	URLScheme scheme = parser.GuessScheme();
	
	bool shouldCache = false;

	switch (scheme) {
		case kHTTP:
#ifndef NOSSL
		case kHTTPS:
#endif
		case kFTP:
			shouldCache = true;
			resource = ReadFromNet(mgr, parser, NULL, docRef, downloadPath, listener, referrer, forceDownload, rangeBegin, useStreamIO, dontReuse);
			break;
			
#ifdef NOSSL
		case kHTTPS:
			errorMessage = kErrorSSLNotImplemented;
			break;		
#endif

		case kNETPOSITIVE:
#ifdef PLUGINS
			if (strncmp(parser.Path(), "Plug-ins/", 9) == 0)
				resource = PluginResource(parser, docRef, downloadPath, listener, useStreamIO);
			else
#endif
			resource = ReadLocal(parser,docRef, downloadPath, listener, useStreamIO);
			break;
			
		case kFILE: {
			const char *hostName = parser.HostName();
			if (hostName && *hostName && (strcmp(hostName,"localhost") != 0))
				resource = ReadFromNet(mgr, parser, NULL, docRef, downloadPath, listener, referrer, false, rangeBegin, useStreamIO, dontReuse);	// ftp:
			else
				resource = ReadFromDisk(parser,docRef, downloadPath, listener, useStreamIO);			// file:
			if (!resource) {
				errorMessage = GetErrorFileNotFound();
				errorMessage.ReplaceFirst("%s", url.String());
			}
			break;
		}
		default: {
			errorMessage = kErrorUnknownURLType;
			BString urlType = url;
			//char *p = strchr(urlType.String(), ':');
			//if (p)
			//	urlType.Truncate(urlType.String() - p);
			int32 loc = urlType.FindFirst(':');
			if (loc > 0)
				urlType.Truncate(loc);
			errorMessage += urlType;
		}
	}
	
	if (resource == NULL) {
		pprint("GetUResource: Can't get '%s'",url.String());
		if (errorMessage.Length() == 0) {
			errorMessage = GetErrorGeneric();
			errorMessage.ReplaceFirst("%s", url.String());
		}
	} else if (shouldCache && !UResourceCache::Cached(url.String(), true))
		UResourceCache::Add(resource);						// Add to cache
		
	return resource;
}

//	Post form data, get a resource

UResourceImp *GetUResourceFromForm(ConnectionManager *mgr, BString& url, BString* post, long docRef, 
								BString &errorMessage, BMessenger *listener, const char *referrer)
{
	UResourceImp* resource = NULL;
	
//	Get the resource from the right place...

//	NP_ASSERT(strstr(url.String(),"http://"));

// Ron says: DON'T CACHE FORM OUTPUT!
//	if (resource = UResourceCache::Cached(url))
//		return resource;

	URLParser parser;
	parser.SetURL(url.String());
	resource = ReadFromNet(mgr, parser, post, docRef, NULL, listener, referrer, false, 0, false, false);		//	http:
	
	if (resource == NULL)
		pprint("GetUResourceFromForm: Can't get '%s'",url.String());
// Ron says: DON'T CACHE FORM OUTPUT!
// But it does the right thing and sets it to expired.
	else if (!UResourceCache::Cached(url.String(), true))
		UResourceCache::Add(resource);

	errorMessage = GetErrorGeneric();
	errorMessage.ReplaceFirst("%s", url.String());

	return resource;
}


//	Create a new resource from data

UResourceImp* NewResourceFromData(const void* data, int length, const BString& url, const BString& type, BMessenger *listener, bool useStreamIO)
{
	Store *store = new BucketStore();
	store->Write(data,length);
	UResourceImp *imp = new UResourceImp(url.String(), store, listener, NULL, useStreamIO);
	imp->SetContentType(type.String());
	imp->SetContentLength(length);
	imp->NotifyListeners(msg_ResourceChanged);
	imp->Flush(kComplete);
//	return new UResource(imp);
	imp->RefCount(1);
	if (useStreamIO) {
		UResourceImp *copy = CopyResourceToStream(imp);
		imp->RefCount(-1);
		return copy;
	}
	return imp;
}


//===========================================================================
//===========================================================================
//	UResourceImp implements the resource, contains the data etc.

long GetDefaultExpiration()
{
	// This routine is called a lot and is a bit inefficient, so we'll
	// cache the value and update it at most once a minute.
	static bigtime_t lastTime = 0;
	static long cachedExpiration = 0;
	bigtime_t now = time(NULL);
	
	if ((now - lastTime) < ONE_SECOND * 60) {
		return cachedExpiration;
	}
	
	lastTime = now;

	switch(UResourceCache::GetCacheOption()) {
		case 0: // Every time
			return (cachedExpiration = now);
		case 1: // Once per session
			return (cachedExpiration = -1);
		case 2: { // Once per day
			tm *t;
			time_t now = time(NULL);
			now += 3600 * 24;
			t = localtime(&now);
			t->tm_sec = t->tm_min = t->tm_hour = 0;
			return (cachedExpiration = mktime(t));
		}
		case 3: // Never
		default:
			return (cachedExpiration = 0x7fffffff);
	}
}

long UResourceImp::sEarliestVisited    = 0x7fffffff;
long UResourceImp::sLatestVisited      = 0;
long UResourceImp::sGreatestUsageCount = 1;

UResourceImp::UResourceImp(const char* url, Store *store, BMessenger *messenger, const char *path, bool usesStream)
{
//printf("%5d Const     0x%x\n", find_thread(NULL), this);
	mUsesStream = usesStream;
	mDead = false;
	mURL = 0;
	mFragment = 0;
	mContentType = 0;
	mContentLength = 0;
	mCacheName = 0;
	mCacheOnDisk = false;
	mDate = 0;
	mLastModified = 0;
	mExpires = 0;
	mLastVisited = 0;
	mUpToDate = false;
	mUsageCount = 0;
	mEntry = NULL;
	mRefCount = 0;
	mStatus = kIdle;
//	mURL = SetStr((char*)mURL,(char*)url);
	SetURL(url);
	mStore = store;
	// If the URL contains a question mark, then it means it's generated by a CGI.  In general, CGI-generated pages
	// shouldn't be cached, so we'll set their expiration to now.  However, operations that force it to use the cache,
	// like going forward and back or saving to disk, will be able to use the cached copy.
	if (strchr(url, '?') != 0)
		mExpires = time(NULL);
	else
		mExpires = GetDefaultExpiration();
		
	if (path != NULL)
		mEntry = new BEntry(path);
	
	if (messenger) {
		RefCount(1);
		mMessengers.AddItem(messenger);
	}
}

UResourceImp::~UResourceImp()
{
//printf("%5d Dest      0x%x\n", find_thread(NULL), this);
	if (mRefCount != 0)
		mRefCount = 0;	// debugger
	if (mURL) {
		free(mURL);
		mURL = NULL;
	}
	if (mContentType) {
		free(mContentType);
		mContentType = NULL;
	}
	if (mCacheName) {
		free(mCacheName);
		mCacheName = NULL;
	}
	if (mStore) {
		delete mStore;
		mStore = NULL;
	}
	
	if (mEntry)
		delete mEntry;
}

#ifdef NPOBJECT
const char*	UResourceImp::GetInfo()				// Info to help track object
{
	return mURL;
}
#endif

//	Add or remove a reference to this resourceImp
//	If it is not being used and is dead or not to be cached, delete it


int UResourceImp::RefCount(long delta)
{
	int32 new_value = atomic_add(&mRefCount, delta) + delta;
	if (new_value <= 0 && (mDead || GetCacheName() == NULL)) {	// Refcount == 0, resourceImp is dead, get rid of it
		UResourceCache::Delete(this);
		return 0;
	} else if (new_value <= 0) {
		// The resource is cached, but no longer referenced.  Let's free up its store
		// and the semaphore it uses.
		SetStore(NULL);
	}

	return new_value;
}

const char*	UResourceImp::Key(int& length)
{
//	NP_ASSERT(mURL);
	length = strlen(mURL);
	return mURL;
}

bool UResourceImp::Lock()
{
	return mLocker.Lock();
}

bool UResourceImp::IsLocked()
{
	return mLocker.IsLocked();
}

status_t UResourceImp::LockWithTimeout(bigtime_t timeout)
{
	return mLocker.LockWithTimeout(timeout);
}

void UResourceImp::Unlock()
{
	mLocker.Unlock();
}

void UResourceImp::MarkForDeath()
{
	mDead = true;								// ResourceImp is dead, expired or generally useless
}

long UResourceImp::CreateStore()				// Called once HTTP gets to body of data, may never happen
{
	delete mStore;
	if (mUsesStream) {
#ifdef PLUGINS
		mStore = new StreamStore(this);
#endif
	} else if (mEntry == NULL) {
		if (mContentLength)
			mStore = new BucketStore(mContentLength);
		else
			mStore = new BucketStore;					// Get a new caching store for the incoming data
	} else {
		mStore = new BFileStore(mEntry);
		if (mStore && mContentType)
			((BFileStore *)mStore)->SetType(mContentType);
		if (mStore && mURL)
			((BFileStore *)mStore)->SetSourceURL(mURL);
	}
	return mStore ? 0 : -1;						// Failed to create store?
}

void UResourceImp::SetDownloadPath(const char *path, bool forceEntireCopy)
{
	delete mEntry;
	mEntry = new BEntry(path);
	if (!mStore) {
		// That was easy; we haven't created the store yet.  Just set the new path so
		// we'll be ready.
	} else {
		// Sigh.  We've already started the download.  Create the new store, copy the old bits into
		// it, and switch stores.
		Store *oldStore = mStore;
//		oldStore->Lock();
		Store *newStore = new BFileStore(mEntry);
		if (newStore && mContentType)
			((BFileStore *)newStore)->SetType(mContentType);
		if (newStore && mURL)
			((BFileStore *)newStore)->SetSourceURL(mURL);
		long pos = 0;
		if(!forceEntireCopy)
			pos = newStore->GetLength();
		while (pos < oldStore->GetLength()) {
			long size = MIN(1024, oldStore->GetLength() - pos);
			void *ptr = oldStore->GetData(pos, size);
			if (ptr)
				newStore->Write(ptr, size);
			pos += size;
			oldStore->ReleaseData(ptr, size);
		}
		mStore = newStore;
		delete oldStore;
	}
}

void UResourceImp::SetStore(Store *store)
{
	delete mStore;
	mStore = store;
}

Store* UResourceImp::GetStore()
{
	return mStore;
}

const char*	UResourceImp::GetURL()
{
	return mURL;
}

const char*	UResourceImp::GetFragment()
{
	return mFragment;	// Lookup fragment in url
}

const char*	UResourceImp::GetContentType()
{
	return mContentType;
}

uint32 UResourceImp::GetContentLength()
{
	return mContentLength;
}

long UResourceImp::GetDate()
{
	return mDate;
}

long UResourceImp::GetLastVisited()
{
	return mLastVisited;
}

long UResourceImp::GetLastModified()
{
	return mLastModified;
}

long UResourceImp::GetExpires()
{
	return mExpires;
}

StoreStatus UResourceImp::GetProgress(long *done, long *of)
{
	*done = GetLength();
	*of = GetContentLength();
	return GetStatus();
}


void UResourceImp::SetStatus(StoreStatus status)
{
	mStatus = status;
}

//	What purpose does the status serve?

StoreStatus UResourceImp::GetStatus()
{
	if (mDead) {
		return kError;
	}

	if (mStatus == kError || mStatus == kAbort)
		return (StoreStatus)mStatus;

	if (mStore)
		return mStore->GetStatus();	

	return (StoreStatus)mStatus;
}

uint32 UResourceImp::GetLength()
{
	return mStore ? mStore->GetLength() : 0;
}

const char*	UResourceImp::GetCacheName()
{
	return mCacheName;
}

bool UResourceImp::GetCacheOnDisk()
{
	return mCacheOnDisk;
}

void* UResourceImp::GetData(long pos, long size)
{
	if (!Lock()) return NULL;
	void* result = mStore ? mStore->GetData(pos,size) : NULL;
	if (result == NULL)
		Unlock();			// Won't call release data if result is NULL

	return result;
}

long UResourceImp::ReleaseData(void *data, long size)
{
	long result =  mStore ? mStore->ReleaseData(data,size) : 0;
	Unlock();
	return result;
}

bool
UResourceImp::IsUpToDate()
{
	return (mUpToDate);
}

void
UResourceImp::SetUpToDate(
	bool	uptodate)
{
	mUpToDate = uptodate;
}

long UResourceImp::Write(const void *data, long count, bool notifyListeners)
{
	if (mDead)
		return -1;
	if (mStore == NULL)
		CreateStore();
	UResourceCache::AddToCacheSize(count);
	long val = mStore ? mStore->Write(data,count) : 0;
	if (notifyListeners && count > 0)
		NotifyListeners(msg_ResourceChanged);
	return val;
}

void UResourceImp::Seek(uint32 pos)
{
	if (mStore == NULL)
		CreateStore();
	if (mStore)
		mStore->Seek(pos);
}

void UResourceImp::NotifyListeners(uint32 msgID, bool synchronous)
{
	if (mMessengers.CountItems() > 0) {
		BMessage msg(msgID);
		BMessage reply;
		msg.AddPointer("ResourceImp", this);
		for (int i = 0; i < mMessengers.CountItems(); i++) {
			if (synchronous)
				((BMessenger *)mMessengers.ItemAt(i))->SendMessage(&msg, &reply);
			else
				((BMessenger *)mMessengers.ItemAt(i))->SendMessage(&msg);
		}
	}
}

void UResourceImp::NotifyListenersOfSwitch(UResourceImp *newImp)
{
	if (mMessengers.CountItems() > 0) {
		BMessage msg(msg_ResourceSwitched);
		BMessage reply;
		msg.AddPointer("OldImp", this);
		msg.AddPointer("NewImp", newImp);
		for (int i = 0; i < mMessengers.CountItems(); i++)
			((BMessenger *)mMessengers.ItemAt(i))->SendMessage(&msg);
	}
}

void UResourceImp::AddListener(BMessenger *listener)
{
	if (!mMessengers.HasItem(listener)) {
		RefCount(1);
		mMessengers.AddItem(listener);
	}
}

void UResourceImp::RemoveListener(BMessenger *listener)
{
	if (mMessengers.HasItem(listener)) {
		if (mMessengers.RemoveItem(listener))
			RefCount(-1);
	}
}

void UResourceImp::InheritListeners(UResourceImp *imp)
{
	for (int i = 0; i < imp->mMessengers.CountItems(); i++) {
		BMessenger *messenger = (BMessenger*)imp->mMessengers.ItemAt(i);
		if (messenger) {
			AddListener(messenger);
		}
	}
}

void UResourceImp::Flush(StoreStatus status)
{
	if (!Lock()) return;
	if (mStore)
		mStore->Flush(status);
	
		
	if (mStore && status == kComplete) {
		if (mCacheOnDisk) {
			UResourceCache::CacheOnDisk(this);	// Commit to disk cache
		}
		mStore->Close();
	} else {
		mDead = true;							// Got an error, kill when refcount == 0
	}

	if (GetContentLength() < GetLength())
		pprint("UResourceImp::Flush: Length greater than expected .. %d,%d",GetLength(),GetContentLength());
		
	mCacheOnDisk = false;
	mStatus = status;
	Unlock();
	NotifyListeners(msg_ResourceFlushed);
}

void UResourceImp::SetURL(const char *url)
{
	mFragment = 0;
	mURL = SetStr((char*)mURL,(char*)url);
	if ((bool)(mFragment = strrchr(mURL,'#'))) {
		mFragment[0] = 0;					// Crop fragment from url
		mFragment++;
	}
}

void UResourceImp::SetContentType(const char *type)
{
	mContentType = SetStr((char*)mContentType,(char*)type);
}

void UResourceImp::SetContentLength(long length)
{
	mContentLength = length;
}

void UResourceImp::SetDate(long date)
{
	mDate = date;
}

void UResourceImp::SetLastModified(long date)
{
	mLastModified = date;
}

void UResourceImp::SetExpires(long date)
{
	long defExp = GetDefaultExpiration();
	if (date > 0)
		if (defExp == -1)
			mExpires = defExp;
		else
			mExpires = MIN(date, defExp);
}

void UResourceImp::SetLastVisited(long date)
{
	mLastVisited = date;
	if (date < sEarliestVisited)
		sEarliestVisited = date;
	if (date > sLatestVisited)
		sLatestVisited = date;
}

void UResourceImp::SetUsageCount(long count)
{
	mUsageCount = count;
	if (count > sGreatestUsageCount)
		sGreatestUsageCount = count;
}

void UResourceImp::IncrementUsageCount()
{
	mUsageCount++;
}

void UResourceImp::SetCacheName(const char *cacheName)
{
	mCacheName = SetStr((char*)mCacheName,(char*)cacheName);
}

void UResourceImp::SetCacheOnDisk(bool cacheOnDisk)
{
	mCacheOnDisk = cacheOnDisk;
}

void UResourceImp::RecalcCacheScore()
{
	// Recalculate the cache score.  Resources with a higher score are more likely
	// to stay in the cache; resources with a lower score are more likely to be
	// discarded.
	
	// Caching has the following priorities:
	
	// Resources that are not up to date automatically get a score of zero.
	// Resources more recently visited get an increased score.
	// Resources more frequently visited get an increased score.
	// Resources larger than 8K that are frequently and recently visited get an increased score.
	// Resources larger than 8K that are neither frequently nor recently visited get a decreased score.
	// Other than the 8K criterion, the score is not based on resource size.
	
	// When comparing the visited and modified dates, the dates are compared to the earliest
	// visitation and modification dates of the entries that are in the cache when the program
	// starts up.  Number of visits are normalized to the cache entry with the greatest number of
	// visits.	This gives us a stable basis for comparison throughout program execution.  These
	// values are not updated if the cache entries upon which they are based are removed
	// from the cache or updated, so it is possible that the calculations will become slightly
	// skewed during program execution.  However, this should not cause many ill effects.

	if (!mUpToDate) {
		mCacheScore = 0.0;
	} else {
		float visitationScore = (1.0 + ((float)(mLastVisited - sEarliestVisited)) / ((float)(sLatestVisited - sEarliestVisited) + 1.0));
		mCacheScore = ( visitationScore *
					   (1.0 + (((float)mUsageCount) / ((float)sGreatestUsageCount))));
		if (mContentLength >= 8192) {
			float newVisitation = log(visitationScore + 2.5);
			float newUsage = log((float)mUsageCount / 2.0 + 2.0);
			mCacheScore = mCacheScore * newVisitation * newUsage;
		}
	}
}


//===========================================================================
//===========================================================================
//	Consumer

Consumer::Consumer() : BLooper("Consumer", B_DISPLAY_PRIORITY - 1)
{
	mResourceImp = 0;
	mConsumed = 0;
	mError = 0;
	mMessenger = this;
	mComplete = false;
	mWantsQuit = false;
	mNeedsToProcessData = false;
	mWaitingForListenerTime = 0;
pprint("Consumer 0x%x constructor", this);
}

Consumer::~Consumer()
{
	if (mResourceImp)
		mResourceImp->RemoveListener(&mMessenger);
	NetPositive::RemoveThread(Thread());
}

void Consumer::Kill()
{
pprint("Consumer 0x%x kill", this);
	mWantsQuit = true;
	BMessage msg(msg_ConsumerWantsQuit);
	msg.AddPointer("Consumer", this);
	for (int i = mListeners.CountItems() - 1; i >= 0; i--) {
		((BMessenger *)mListeners.ItemAt(i))->SendMessage(&msg);
		mListeners.RemoveItem(i);
	}
}

void Consumer::SetResourceImp(UResourceImp *resourceImp)
{
pprint("Consumer 0x%x SetResourceImp", this);
	mResourceImp = resourceImp;
	if (!resourceImp)
		return;
		
	resourceImp->AddListener(&mMessenger);
	BMessage msg(msg_ResourceChanged);
	mMessenger.SendMessage(&msg);
	NetPositive::AddThread(Run());
#ifdef DEBUGMENU
	URLParser parser;
	parser.SetURL(resourceImp->GetURL());
	if (Thread() && parser.Path())
		rename_thread(Thread(), parser.Path());
#endif
}

UResourceImp *Consumer::GetResourceImp()
{
	return mResourceImp;
}

bool Consumer::AddListener(BMessenger *listener)
{
pprint("Consumer 0x%x AddListener", this);
	mListeners.AddItem(listener);
	return !mWantsQuit;
/*
	if (mWantsQuit) {
		BMessage msg(msg_ConsumerWantsQuit);
		msg.AddPointer("Consumer", this);
		listener->SendMessage(&msg);
	}
*/
}

void Consumer::Reset()
{
	mConsumed = 0;
	SetError(false);
	SetComplete(false);
	BMessage msg(msg_ResourceChanged);
	mMessenger.SendMessage(&msg);
}

long Consumer::Write(uchar *, long count)
{
	return count;
}

void Consumer::SetComplete(bool complete)
{
	mComplete = complete;
}

void Consumer::SetError(bool error)
{
	mError = error;
	if (error)
		Kill();
}


void Consumer::MessageReceived(BMessage *msg)
{
	switch(msg->what) {
		case msg_ResourceChanged:
		case msg_ResourceFlushed:
			if (!mNeedsToProcessData) {
				PostMessage(msg_DoSomeWork);
				mNeedsToProcessData = true;
			}
			break;
			
		case msg_DoSomeWork:
			GotData();
			mNeedsToProcessData = false;
			break;
			
		case msg_ResourceSwitched:
			msg->FindPointer("NewImp", (void **)&mResourceImp);
			break;
			
		default:
			BLooper::MessageReceived(msg);
			break;
	}
}


void Consumer::GotData()
{
	if (mListeners.CountItems() == 0) {
		// We don't have any listeners.  Wait for five seconds to see if we get one
		// and if we don't, then give up and quit.
		if (mWaitingForListenerTime == 0)
			mWaitingForListenerTime = system_time();
		else if ((system_time() - mWaitingForListenerTime) > ONE_SECOND * 120) {
//			SetError(true);
			PostMessage(B_QUIT_REQUESTED);
		}
	}
	
	if (mComplete || mWantsQuit) {
		return;
	}

		long	currentSize,expectedSize;
		bool	result = false;
		long	size = 0,write = 0,q=0;
		void*	data;
		
		if (!mResourceImp || mError) {
			pprint("Punting in error");
			return;
		}
			
		if (status_t status = mResourceImp->LockWithTimeout(TEN_SECONDS) != B_OK) {
			// Couldn't acquire lock on resource, try again.
			return;
		}

		mResourceImp->GetProgress(&currentSize,&expectedSize);	
		do {
			if (mWantsQuit) return;
			size = currentSize - mConsumed;
			data = mResourceImp->GetData(mConsumed,size);		// Get size bytes of data in memory
			if (data == NULL) {
				continue;
			}

			long q2 = 0;
			do {
				write = Write((uchar *)data + q2,size - q2);				// Write some of them to consumer
				if (write > 0)
					q2 += write;
			} while (write > 0 && q2 < size);
			
			mResourceImp->ReleaseData(data,size);				// Relase data so buffer can refil...
			if (q2 > 0) {
				mConsumed += q2;
				q += q2;
				result = true;
			}
		} while (write > 0);
			
		BMessage msg2(msg_ConsumerUpdate);
		long	curSize, of;
		StoreStatus status = mResourceImp->GetProgress(&curSize,&of);
		if (status == kComplete && mConsumed < curSize)
			status = kIdle;

		// Tell a white lie when reporting the status back to the HTMLWorker.
		// We will report the amount complete as being how much of the resource
		// is loaded, not how much is consumed.  If everything is loaded, we'll
		// say that we're kComplete, even if there is data left to consume.  HTMLWorker
		// only uses this to update the page loading indicator, not to make decisions,
		// so it won't hurt.
		if (curSize == of)
			status = kComplete;
		msg2.AddInt32("Status", status);
		msg2.AddInt32("Done", curSize);
		msg2.AddInt32("Of", of);
		msg2.AddPointer("Consumer", this);
		
// Moved below.
//		for (int i = 0; i < mListeners.CountItems(); i++) {
//			((BMessenger *)mListeners.ItemAt(i))->SendMessage(&msg2);
//		}

		if (write < 0) {
			SetError(true);
			pprint("Consumer 0x%lX: Error at %d of '%s' (Current: %d, Expected %d)",(long)this,mConsumed,mResourceImp->GetURL(),currentSize,expectedSize);
		}
		
		bool isComplete = InternalComplete();
		SetComplete(isComplete);

		for (int i = 0; i < mListeners.CountItems(); i++) {
			((BMessenger *)mListeners.ItemAt(i))->SendMessage(&msg2);
		}


		if (isComplete) {
			BMessage msg(msg_ConsumerFinished);
			msg.AddPointer("Consumer", this);
			for (int i = 0; i < mListeners.CountItems(); i++)
				((BMessenger *)mListeners.ItemAt(i))->SendMessage(&msg);
		}
		mResourceImp->Unlock();

		if (!mComplete && mConsumed < currentSize) {
			// It fell out of the loop because our Write call returned zero, but there
			// are still bytes in the buffer left to consume.  Let's snooze a bit and
			// retry the write.
			snooze(50000);
			PostMessage(msg_ResourceChanged);
		}
}

bool Consumer::Complete()
{
	return mComplete;
}

//	See if the consumer is done consuming

bool Consumer::InternalComplete()
{
	if (mError)
		return true;
		
	if (mResourceImp == NULL)
		return false;
		
	long currentSize,expectedSize;
	if (mResourceImp->LockWithTimeout(TEN_SECONDS) != B_OK)
		return false;

	StoreStatus status = mResourceImp->GetProgress(&currentSize,&expectedSize);
	mResourceImp->Unlock();
	if (status == kError || status == kAbort || (status == kComplete && mConsumed == currentSize)) {
		return true;
	}
	if (expectedSize == 0)
		return false;

	return (mConsumed > 0 && mConsumed >= expectedSize);
}


extern ConnectionManager *gConnectionMgr;

ResourceIO::ResourceIO(const char *url)
	: mPosition(0)
{
	BString msg;
	mResource = GetUResource(gConnectionMgr, url, 0, msg);
}


ResourceIO::~ResourceIO()
{
	if (mResource)
		mResource->RefCount(-1);
}


ssize_t ResourceIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	if (!mResource)
		return 0;
		
	off_t end = mResource->GetContentLength();
	long done, of;
	mResource->GetProgress(&done, &of);

	if (pos > end || pos < 0)
		return B_ERROR;
		
	size = MIN(size, end - pos);
	
	while (done < pos + size) {
		snooze(100000);	
		mResource->GetProgress(&done, &of);
	}
	
	void *data = mResource->GetData(pos, size);
	if (!data)
		return 0;
		
	memcpy(buffer, data, size);
	mResource->ReleaseData(data, size);
	
	return size;
}


off_t ResourceIO::Seek(off_t position, uint32 seek_mode)
{
	if (!mResource)
		return 0;
	
	off_t end = mResource->GetContentLength();
	
	switch(seek_mode) {
		case SEEK_SET:
			mPosition = position;
			break;
		case SEEK_CUR:
			mPosition += position;
			break;
		case SEEK_END:
			mPosition = end + position;
			break;
	}
	
	mPosition = MAX(MIN(mPosition, end), 0);
	
	return mPosition;
}


off_t ResourceIO::Position() const
{
	return mPosition;
}
