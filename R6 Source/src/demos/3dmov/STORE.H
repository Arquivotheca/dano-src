// ===========================================================================
//	Store.cpp
// 	©1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#ifndef __STORE__
#define __STORE__

#include "UTILS.H"

//=======================================================================
//	Ring buffer for streaming data

class Buffer {
public:
					Buffer(long bufferSize = 0x1000);
	virtual			~Buffer();
	
			void	Reset();
			
	virtual	long	ReadLine(char *line);			// Buffer methods	
	
	virtual	void*	GetReadBuffer(long* maxSize);
	virtual	void	ReadBuffer(long size);
	
	virtual	void*	GetWriteBuffer(long* maxSize);
	virtual	void	WriteBuffer(long size);
		
protected:
			char*	mBuffer;
			long	mBufferSize;
			long	mBufferMask;
			long	mHead;
			long	mTail;
};

//====================================================================
//	Somewhere to read and write data

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

class Store : public NPObject {
protected:
public:
	virtual				~Store();
						
	
	virtual	long		Open(bool readOnly);				// Store has a File like interface
	virtual	long		Close();
	
	virtual	long		GetLength();
	virtual	long		GetPos();
	virtual	long		Seek(long pos);
	
	virtual	long		Read(void *data, long count);
	virtual	long		Write(void *data, long count);


	virtual	StoreStatus	GetStatus();						// Current store status
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete
	
	virtual	void*		GetData(long pos, long size);		// Cached read methods
	virtual	long		ReleaseData(void *data, long size);
	
	virtual	long		Delete();							// Delete hard copy of data
	virtual	void		GetURL(char *url);
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
	virtual	long		Write(void *data, long count);


	virtual	StoreStatus	GetStatus();						// Current store status
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete
	
	virtual	void*		GetData(long pos, long size);		// Cached read methods
	virtual	long		ReleaseData(void *data, long size);
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
	virtual				~BucketStore();
	
	virtual	long		GetLength();
	virtual	long		Write(void *data, long count);


	virtual	StoreStatus	GetStatus();						// Current store status
	virtual	void		Flush(StoreStatus status);			// Flush when store is complete
	
	virtual	void*		GetData(long pos, long size);		// Cached read methods
protected:
			StoreStatus	mStatus;
			CBucket		mBucket;
};

#endif
