// ===========================================================================
//	Store.cpp
// 	©1996 by Peter Barrett, All rights reserved.
// ===========================================================================

#include "STORE.H"

#undef NP_ASSERT
#define NP_ASSERT

//=======================================================================
//	Ring buffer for streaming data

Buffer::Buffer(long bufferSize) : mHead(0),mTail(0)
{
	ulong size = 1;
	while (size < bufferSize)
		size <<= 1;
	NP_ASSERT(size == bufferSize);	// Must be a power of two
	mBufferSize = size;
	mBufferMask = size - 1;
	mBuffer = (char *)MALLOC(size);
}

Buffer::~Buffer()
{
	if (mBuffer)
		FREE(mBuffer);
}

//	

void Buffer::Reset()
{
	mHead = 0;
	mTail = 0;
}

//	Returns address of data to read from buffer and its size

void* Buffer::GetReadBuffer(long *maxSize)
{
	if ((mTail & mBufferMask) > (mHead & mBufferMask))	// Only return contig space
		*maxSize = mBufferSize - (mTail & mBufferMask);
	else
		*maxSize = mHead - mTail;

	NP_ASSERT(mBuffer);
	NP_ASSERT(mTail <= mHead);
	NP_ASSERT(*maxSize <= mBufferSize);
	return mBuffer + (mTail & mBufferMask);
}

void Buffer::ReadBuffer(long size)
{
	NP_ASSERT(mBuffer);
	mTail += size;
}

//	Returns address of write position in buffer and size of contiguous space

void* Buffer::GetWriteBuffer(long *maxSize)
{
	NP_ASSERT(mBuffer);
	if ((mHead & mBufferMask) >= (mTail & mBufferMask))	// Only return contiguous space
		*maxSize = mBufferSize - (mHead & mBufferMask);
	else
		*maxSize = mBufferSize - (mHead - mTail);
		
	NP_ASSERT(mTail <= mHead);
	NP_ASSERT(*maxSize <= mBufferSize);
	return mBuffer + (mHead & mBufferMask);
}

void Buffer::WriteBuffer(long size)
{
	NP_ASSERT(mBuffer);
	mHead += size;
}

//	Read a line of data, line end is defined by CRLF, CR or LF

long Buffer::ReadLine(char *line)
{
	char	c,other;
	long	p;

	NP_ASSERT(mBuffer);
	p = mTail;
	while (p < mHead) {
		c = (*line++ = mBuffer[p++ & mBufferMask]);
		if ((c == 0x0D) || (c == 0x0A)) {
			line[-1] = 0;		// Got a complete line, strip the 0x0D
			if (p == mHead)
				break;			// Bounary case, can't have readTail > readHead
			
			other = mBuffer[p & mBufferMask];
			if (((other == 0x0D) || (other == 0x0A)) && other != c)	// Don't strip 0x0A,0x0A or 0x0D,0x0D
				p++;			// And the 0x0A in in 0x0D,0x0A
			mTail = p;		
			return 0;
		}
	}
	return 1;					// No line yet, but not an error
}

//=======================================================================
//	Store is an abstract base class?

Store::~Store()
{
}

long Store::Open(bool readOnly)
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

long Store::Seek(long pos)
{
	return 0;
}

long Store::Read(void *data, long count)
{
	return 0;
}

long Store::Write(void *data, long count)
{
	return 0;
}

StoreStatus Store::GetStatus()
{
	return kUninitialized;
}

void Store::Flush(StoreStatus status)
{
}

void* Store::GetData(long pos, long size)
{
	return 0;
}

long Store::ReleaseData(void *data, long size)
{
	return 0;
}

long Store::Delete()
{
	return 0;
}

void Store::GetURL(char *url)
{
	url[0] = 0;
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
		FREE(mGetDataBuffer);
	if (mData)
		FREE(mData);
}

//	Read into memory if its small on open

long BufferedStore::Open(bool readOnly)
{
	long result;
	
	if (readOnly && mLength < (32*1024L)) {		//	Just read small files into memory....
		if ((result = mFile->Open(readOnly)) == 0) {
			mData = MALLOC(mLength);
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
		NP_ASSERT(mData);
		char* s = (char*)mData + mDataPos;
		memcpy(data,s,MIN(mLength - mDataPos,count));
		return 0;
	}
	return mFile->Read(data,count);
}

long BufferedStore::Write(void *data, long count)
{
	NP_ASSERT(mFile);
	return mFile->Write(data,count);
}

StoreStatus	BufferedStore::GetStatus()					// Current store status
{
	return kComplete;
}

void BufferedStore::Flush(StoreStatus status)			// Flush when store is complete
{
}

void* BufferedStore::GetData(long pos, long size)		// Cached read methods
{
	if (mLength == 0)
		return NULL;
		
	if (mData)
		return (Byte *)mData + pos;	// In memory
	
//	See if the request is in the buffer

	mGetDataBuffer = MALLOC(size);
	if (mGetDataBuffer) {
		mFile->Seek(pos);
		mFile->Read(mGetDataBuffer,size);
	}
	return mGetDataBuffer;
}

long BufferedStore::ReleaseData(void *data, long size)
{
	if (mGetDataBuffer && mGetDataBuffer == data) {
		FREE(mGetDataBuffer);
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

long BucketStore::Write(void *data, long count)
{
	mBucket.AddData(data,count);
	return 0;
}

void* BucketStore::GetData(long pos, long size)
{
	return mBucket.GetData() + pos;
}

