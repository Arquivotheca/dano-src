// ===========================================================================
//	UResource.h
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1995 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __URESOURC__
#define __URESOURC__

#include "HashTable.h"

#include <OS.h>
#include <List.h>
#include <Locker.h>
#include <Looper.h>
#include <Messenger.h>
#include <String.h>
#include <DataIO.h>
class BEntry;

class ConnectionManager;
class Store;

const uint32 msg_ResourceChanged = 'Rchg';
const uint32 msg_ResourceFlushed = 'Rfls';
const uint32 msg_ResourceSwitched = 'Rswt';
const uint32 msg_AddConsumer = 'Acns';
const uint32 msg_ConsumerUpdate = 'Cupd';
const uint32 msg_ConsumerWantsQuit = 'Cwqt';
const uint32 msg_ConsumerFinished = 'Cfsh';
const uint32 msg_ViewUpdate = 'Vupd';
const uint32 msg_RequestViewUpdate = 'Rvup';
const uint32 msg_DoSomeWork = 'Dswk';

void DetermineFileType(const char *filename, ulong fileType, const char *data, int dataCount, char *typeStr);

enum StoreStatus {
	kUninitialized,
	kIdle,

	kDNS,		// Before DNS returns
	kConnect,	// Before Connect
	kRequest,	// After Connect, before data starts flowing
	
	kInProgress,
	kComplete,
	
	kLoadingHTML,
	kLoadingImages,
	
	kTimeout,
	kAbort,
	kError
};

class UResourceImp;

//===========================================================================

UResourceImp*	GetUResource(ConnectionManager *mgr, const BString& url, long docRef, BString &errorMessage, 
						 bool forceCache = false, const char *downloadPath = NULL, BMessenger *listener = NULL, const char *referrer = NULL,
						bool forceDownload = false, uint32 rangeBegin = 0, bool useStreamIO = false, bool dontReuse = false);

UResourceImp*	GetUResourceFromForm(ConnectionManager *mgr, BString& url, BString* post, long docRef, 
								 BString &errorMessage, BMessenger *listener = NULL, const char *referrer = NULL);

UResourceImp*	NewResourceFromData(const void* data, int length, const BString& url, const BString& type, BMessenger *listener = NULL, bool useStreamIO = false);

//===========================================================================
//	UResourceImp contains the real resource info and data store
//	The store may be a number of different flavors:
//
//		MemoryStore	-	Created in the heap, no file behind it, temporary
//		FileStore	-	Saved to the disk as a file, an FTP file transfer for example
//		CacheStore	-	Stored in the cache, may be temp
//

class UResourceImp : public HashEntry {
friend class UResourceCache;
public:
					UResourceImp(const char* url, Store *store = 0, BMessenger *listener = 0, const char *path = 0, bool usesStream = false);
					
virtual	const char*	Key(int& length);		// For adding to hash table

virtual	bool		Lock();
virtual	status_t	LockWithTimeout(bigtime_t timeout);
virtual	void		Unlock();
		bool		IsLocked();

		const char*	GetURL();
		const char*	GetFragment();
		const char*	GetContentType();
		uint32		GetContentLength();
		
		long		GetDate();
		long		GetLastModified();
		long		GetExpires();
		long		GetLastVisited();
		long		GetUsageCount() {return mUsageCount;}
		long		GetRefCount()	{return mRefCount;}
		

		const char*	GetCacheName();
		bool		GetCacheOnDisk();
	
		StoreStatus	GetProgress(long *done, long *of);
		void		SetStatus(StoreStatus status);
		StoreStatus	GetStatus();		// Store access methods
		uint32		GetLength();
		
		void*		GetData(long pos, long size);
		long		ReleaseData(void *data, long size);
		long		Write(const void *data, long count, bool notifyListeners = true);
		void		Seek(uint32 pos);
		void		NotifyListeners(uint32 msg, bool synchronous = false);
		void		NotifyListenersOfSwitch(UResourceImp *newImp);
		void		InheritListeners(UResourceImp *imp);
		void		Flush(StoreStatus status);
		
		void		SetURL(const char* url);
		void		SetContentType(const char *type);
		void		SetContentLength(long length);

		void		SetDate(long date);
		void		SetLastModified(long date);
		void		SetExpires(long date);
		void		SetLastVisited(long date);
		void		SetUsageCount(long count);
		void		IncrementUsageCount();
		
		void		SetCacheName(const char *cacheName);
		void		SetCacheOnDisk(bool cacheOnDisk);
		void		RecalcCacheScore();
		float		GetCacheScore() {return mCacheScore;}
		
		int			RefCount(long delta);
		void		MarkForDeath();
virtual long		CreateStore();
		Store*		GetStore();
		void		SetStore(Store *store);
		void		SetDownloadPath(const char *path, bool forceEntireCopy = true);
					//forceEntireCopy is false when the file at path is the result
					//of a tracker copy of the downloading file and the resource
					//shouldn't recopy, but instead copy the difference in file sizes and
					//continue downloading from there.
					
		bool		IsUpToDate();
		void		SetUpToDate(bool uptodate);
		
		void		SetErrorMessage(const BString &message) {mErrorMessage = message;}
		void		GetErrorMessage(BString &message) {message = mErrorMessage;}
		
		void		AddListener(BMessenger *listener);
		void		RemoveListener(BMessenger *listener);
		
		bool		IsDead() {return mDead;}
		BEntry*		GetEntry() {return mEntry;}
		
#ifdef NPOBJECT
virtual	const char*	GetInfo();	// Info to help track object
#endif

protected:
virtual				~UResourceImp();

		int32		mRefCount;		// Number of resources uning this UResourceImp
		
		Store*		mStore;
		char*		mURL;
		char*		mFragment;
		char*		mContentType;
		uint32		mContentLength;
		char*		mCacheName;
		
		long		mDate;
		long		mLastModified;
		long		mExpires;
		long		mLastVisited;
		long		mUsageCount;
		
		float		mCacheScore;
		
		BString		mErrorMessage;
		
		BList 		mMessengers;
		
		BEntry		*mEntry;
		
		TLocker		mLocker;
		
		StoreStatus	mStatus : 4;
		unsigned	mUpToDate : 1;
		unsigned	mCacheOnDisk : 1;	// Write this imp to disk cache when complete
		unsigned	mDead : 1;
		unsigned	mUsesStream : 1;

static	long		sEarliestVisited;
static	long		sLatestVisited;
static	long		sGreatestUsageCount;
};

class ResourceIO : public BPositionIO {
public:
					ResourceIO(const char *url);
virtual				~ResourceIO();

virtual	ssize_t		ReadAt(off_t pos, void *buffer, size_t size);
virtual	ssize_t		WriteAt(off_t /*pos*/, const void* /*buffer*/, size_t /*size*/) {return B_ERROR;}

virtual off_t		Seek(off_t position, uint32 seek_mode);
virtual	off_t		Position() const;

virtual status_t	SetSize(off_t /*size*/) {return B_ERROR;}

protected:
		off_t			mPosition;
		UResourceImp	*mResource;
};

//===========================================================================
//	Consumer monitors a resource and uses its data

class Consumer  : public BLooper {
public:
						Consumer();

	virtual	void		SetResourceImp(UResourceImp *resource);
			UResourceImp*	GetResourceImp();
			
	virtual	void		Reset();							// Server Push reset data
	virtual	long		Write(uchar *data, long count);	// Consume data

	virtual	bool		Complete();
	virtual void		SetComplete(bool complete);
	virtual void		SetError(bool error);
virtual	void			MessageReceived(BMessage *msg);
		bool			AddListener(BMessenger *listener);
virtual void			Kill();

protected:
	virtual				~Consumer();
			bool		InternalComplete();
			void		GotData();

			UResourceImp*	mResourceImp;
			int32		mConsumed;
			int32		mOldConsumed;
			BMessenger	mMessenger;
			BList		mListeners;
			bigtime_t	mWaitingForListenerTime;
			unsigned	mWantsQuit : 1;
			unsigned	mNeedsToProcessData : 1;
			unsigned	mComplete : 1;
			unsigned	mError : 1;
};

#endif
