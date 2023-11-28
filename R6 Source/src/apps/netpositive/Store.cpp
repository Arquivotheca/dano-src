// ===========================================================================
//	Store.cpp
//  Copyright 1998 by Be Incorporated.
// 	Coypright 1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "Store.h"
#include "URL.h"
#include "Protocols.h"
#include "Strings.h"
#include "MessageWindow.h"
#include "NetPositiveStreamIO.h"

#include <SupportDefs.h>
#include <malloc.h>
#include <Path.h>
#include <NodeInfo.h>
#include <stdio.h>

//=======================================================================
//	Store is an abstract base class?

Store::~Store()
{
}

long Store::Open(bool)
{
	return 0;
}

long Store::Close()
{
	return 0;
}

long Store::GetLength()
{
	return 0;
}

long Store::GetPos()
{
	return 0;
}

long Store::Seek(long)
{
	return 0;
}

long Store::Read(void *, long)
{
	return 0;
}

long Store::Write(const void *, long)
{
	return 0;
}

StoreStatus Store::GetStatus()
{
	return kUninitialized;
}

void Store::Flush(StoreStatus)
{
}

void* Store::GetData(long, long)
{
	return 0;
}

long Store::ReleaseData(void *, long)
{
	return 0;
}

long Store::Delete()
{
	return 0;
}

void Store::GetURL(BString& url)
{
	url = "";
}

//=======================================================================
//	Buffered store

BufferedStore::BufferedStore(Store *file)
{	
	mFile = file;
	mBuffer = NULL;
	mData = NULL;
	mGetDataBuffer = 0;
	mDataPos = 0;
	mLength = mFile->GetLength();
}

BufferedStore::~BufferedStore()
{
	delete mFile;
	delete mBuffer;
	if (mGetDataBuffer)
		free(mGetDataBuffer);
	if (mData)
		free(mData);
}

//	Read into memory if its small on open

long BufferedStore::Open(bool readOnly)
{
	long result;
	
	if (readOnly && mLength < (32*1024L)) {		//	Just read small files into memory....
		if ((result = mFile->Open(readOnly)) == 0) {
			mData = malloc(mLength);
			result = mData ? mFile->Read(mData,mLength) : -1;
		}
		delete mFile;
		mFile = 0;
		return result;
	}
	
	return mFile->Open(readOnly);
}

long BufferedStore::Close()
{
	if (mFile)
		return mFile->Close();
	return 0;
}
	
long BufferedStore::GetLength()
{
	return mLength;
}
	
long  BufferedStore::GetPos()
{
	if (mFile == NULL)
		return mDataPos;
	return mFile->GetPos();
}

long BufferedStore::Seek(long pos)
{
	if (mFile == NULL) {
		mDataPos = MIN(pos,mLength);
		return 0;
	}
	return mFile->Seek(pos);
}

//	These guys should never be called?

long BufferedStore::Read(void *data, long count)
{
	if (mFile == NULL) {
//		NP_ASSERT(mData);
		char* s = (char*)mData + mDataPos;
		memcpy(data,s,MIN(mLength - mDataPos,count));
		return 0;
	}
	return mFile->Read(data,count);
}

long BufferedStore::Write(const void *data, long count)
{
//	NP_ASSERT(mFile);
	return mFile->Write(data,count);
}

StoreStatus	BufferedStore::GetStatus()					// Current store status
{
	return kComplete;
}

void BufferedStore::Flush(StoreStatus)			// Flush when store is complete
{
}

void* BufferedStore::GetData(long pos, long size)		// Cached read methods
{
	if (mLength == 0)
		return NULL;
		
	if (mData)
		return (uchar *)mData + pos;	// In memory
	
//	See if the request is in the buffer

	mGetDataBuffer = malloc(size);
	if (mGetDataBuffer) {
		mFile->Seek(pos);
		mFile->Read(mGetDataBuffer,size);
	}
	return mGetDataBuffer;
}

long BufferedStore::ReleaseData(void *data, long)
{
	if (mGetDataBuffer && mGetDataBuffer == data) {
		free(mGetDataBuffer);
		mGetDataBuffer = NULL;
	}
	return 0;
}

//=======================================================================
//	Memory based store

BucketStore::BucketStore()
{
	mStatus = kIdle;
}

BucketStore::BucketStore(long chunkSize) : mBucket(chunkSize)
{
	mStatus = kIdle;
}

BucketStore::~BucketStore()
{
}

long BucketStore::GetLength()
{
	return mBucket.GetCount();
}

StoreStatus BucketStore::GetStatus()
{
	return mStatus;
}

void BucketStore::Flush(StoreStatus status)
{
	mBucket.Trim();
	mStatus = status;
}

long BucketStore::Write(const void *data, long count)
{
	if (data)
		mBucket.AddData(data,count);
	return 0;
}

void* BucketStore::GetData(long pos, long)
{
	return mBucket.GetData() + pos;
}

// ===========================================================================
//	Create a store from a file on the drive...

BFileStore::BFileStore(BEntry *entry) : mStatus(kIdle), mEntry(*entry)
{
	mFile = NULL;
	mLength = 0;
	Open(true);
}

BFileStore::~BFileStore()
{
	if (mFile)
		delete mFile;
}

long BFileStore::Delete()
{
	if (mFile) {
		delete mFile;
		mFile = NULL;
	}
	
	return mEntry.Remove();
}

void BFileStore::GetURL(BString& url)
{
	FileRefToURL(mEntry,url);
}

long BFileStore::Open(bool)
{
	if (!mFile)
		mFile = new BFile(&mEntry,O_RDWR);
	return 0;
}

long BFileStore::Close()
{
	if (mFile) {
		mLength = GetLength();
		delete mFile;
		mFile = NULL;
	}
	return 0;
}

long BFileStore::GetLength()
{
	off_t dataLength;
	if (!mFile)
		return mLength;
	if (mFile->GetSize(&dataLength) == B_NO_ERROR)
		return dataLength;
	return mLength;
}

long BFileStore::GetPos()
{
	if (!mFile)
		return 0;
	return mFile->Position();
}

long BFileStore::Seek(long pos)
{
	if (!mFile)
		return 0;
	off_t sook = mFile->Seek(pos,0);
	return sook;
}

long BFileStore::Read(void *data, long count)
{
	if (!mFile)
		return 0;
	return mFile->Read(data,count);
}

long BFileStore::Write(const void *data, long count)
{
	if (!mFile)
		return 0;
	ssize_t size = mFile->Write(data,count);
	return size;
}

StoreStatus	
BFileStore::GetStatus()
{
	return (mStatus);
}						

void
BFileStore::Flush(
	StoreStatus	status)
{
	mStatus = status;	
}

void BFileStore::SetType(const char *type)
{
	BNode node(&mEntry);
	BNodeInfo info(&node);
	info.SetType(type);
}

void BFileStore::GetType(char *type)
{
	BNode node(&mEntry);
	BNodeInfo info(&node);
	info.GetType(type);
}

void BFileStore::SetSourceURL(const char *sourceURL)
{
	BNode node(&mEntry);
	node.WriteAttr(kBookmarkURLAttr, B_STRING_TYPE, 0, sourceURL, strlen(sourceURL) + 1);
}


#ifdef PLUGINS
StreamStore::StreamStore(UResourceImp *imp)
{
	mStatus = kIdle;
	mStream = NULL;
	mImp = imp;
	mImp->RefCount(1);
	mReadPos = 0;
	mWritePos = 0;
}


StreamStore::~StreamStore()
{
	if (mStream)
		mStream->Dereference();
}


void StreamStore::Flush(StoreStatus status)
{
	if (status != kComplete)
		mStream->SetError(B_ERROR);
	mStatus = status;
	if (mImp)
		mImp->RefCount(-1);
}

StoreStatus StreamStore::GetStatus()
{
	return mStatus;
}


long StreamStore::Write(const void *data, long count)
{
	if (!mStream) {
		mStream = new BNetPositiveStreamIO;
		mStream->SetContentLength(mImp->GetContentLength());
	}
	
	if (!mStream)
		return 0;
		
	long ret = mStream->WriteAt(mWritePos, data, count);
	mWritePos += ret;
	return ret;
}


void* StreamStore::GetData(long pos, long size)
{
	if (!mStream || size == 0)
		return NULL;
		
	long readLength = 0;
	void *buffer = malloc(size);
	
	while (readLength < size) {
		long amtRead = mStream->ReadAt(mReadPos, (char *)buffer + readLength, size - readLength);
		mReadPos += amtRead;
		readLength += amtRead;
		
		if (amtRead == 0) {
			free(buffer);
			return NULL;
		}
	}
	
	return buffer;
}


long StreamStore::ReleaseData(void *data, long size)
{
	if (data)
		free(data);
	return 0;
}


BNetPositiveStreamIO* StreamStore::GetStream()
{
	return mStream;
}
#endif
