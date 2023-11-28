// ===========================================================================
//	Store.cpp
//  Copyright 1998 by Be Incorporated.
// 	Copyright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __STORE__
#define __STORE__

#include "UResource.h"
#include <Locker.h>
#include <Entry.h>
#include <File.h>
#include <String.h>

class Buffer;
class BNetPositiveStreamIO;

//====================================================================
//	Somewhere to read and write data

class Store /*: public NPObject*/ {
protected:
public:
	virtual				~Store();
						
	
	virtual	long		Open(bool readOnly);				// Store has a File like interface
	virtual	long		Close();
	
	virtual	long		GetLength();
	virtual	long		GetPos();
	virtual	long		Seek(long pos);
	
	virtual	long		Read(void *data, long count);
	virtual	long		Write(const void *data, long count);


	virtual	StoreStatus	GetStatus();						// Current store status
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete
	
	virtual	void*		GetData(long pos, long size);		// Cached read methods
	virtual	long		ReleaseData(void *data, long size);
	
	virtual	long		Delete();							// Delete hard copy of data
	virtual	void		GetURL(BString& url);
	
//			bool		Lock() {return mLocker.Lock();}
//			status_t	LockWithTimeout(bigtime_t timeout) {return mLocker.LockWithTimeout(timeout);}
//			void		Unlock()	{mLocker.Unlock();}
//			bool		IsLocked()	{return mLocker.IsLocked() && mLocker.LockingThread() == find_thread(NULL);}
//private:
//	TLocker	mLocker;
};

//====================================================================

class BufferedStore : public Store {
public:
						BufferedStore(Store *file);
	virtual				~BufferedStore();
	
	virtual	long		Open(bool readOnly);				// Store has a File like interface
	virtual	long		Close();
	
	virtual	long		GetLength();
	virtual	long		GetPos();
	virtual	long		Seek(long pos);
	
	virtual	long		Read(void *data, long count);
	virtual	long		Write(const void *data, long count);


	virtual	StoreStatus	GetStatus();						// Current store status
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete
	
	virtual	void*		GetData(long pos, long size);		// Cached read methods
	virtual	long		ReleaseData(void *data, long size);
	
	Store *				GetFile() {return mFile;}
protected:
	
			Store*		mFile;
			Buffer*		mBuffer;
			void*		mData;
			long		mDataPos;
			void*		mGetDataBuffer;
			long		mLength;
};

//====================================================================
//	Bucket Store

class BucketStore : public Store {
public:
						BucketStore();
						BucketStore(long chunkSize);
	virtual				~BucketStore();
	
	virtual	long		GetLength();
	virtual	long		Write(const void *data, long count);


	virtual	StoreStatus	GetStatus();						// Current store status
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete
	
	virtual	void*		GetData(long pos, long size);		// Cached read methods
protected:
			StoreStatus	mStatus;
			CBucket		mBucket;
};

// ===========================================================================
//	Create a store from a file on the drive...

class BFileStore : public Store {
public:		
						BFileStore(BEntry* entry);
						~BFileStore();
						
	virtual	long		Delete();
	virtual	void		GetURL(BString& url);
					
	virtual	long		Open(bool readOnly);
	virtual	long		Close();
	
	virtual	long		GetLength();
	virtual	long		GetPos();
	virtual	long		Seek(long pos);
	
	virtual	long		Read(void *data, long count);
	virtual	long		Write(const void *data, long count);

	virtual	StoreStatus	GetStatus();						// Current store status
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete

	void				SetType(const char *type);
	void				GetType(char *type);
	
	void				SetSourceURL(const char *sourceURL);
	
	BEntry 				GetEntry() {return mEntry;}

protected:
	StoreStatus	mStatus;
	BEntry		mEntry;
	BFile		*mFile;
	long		mLength;
};

#ifdef PLUGINS
class StreamStore : public Store {
public:
						StreamStore(UResourceImp *imp);
	virtual				~StreamStore();
	
	virtual long		Write(const void *data, long count);
	virtual StoreStatus	GetStatus();
	
	virtual void*		GetData(long pos, long size);
	virtual long		ReleaseData(void *data, long size);
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete
	BNetPositiveStreamIO* GetStream();
	
protected:
	StoreStatus				mStatus;
	BNetPositiveStreamIO	*mStream;
	UResourceImp			*mImp;
	
	long					mReadPos;
	long					mWritePos;
};
#endif

#endif
