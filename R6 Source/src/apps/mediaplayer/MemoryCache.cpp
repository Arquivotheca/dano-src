#include <stdio.h>
#include <Debug.h>
#include <Autolock.h>
#include <SupportDefs.h>
#include <errno.h>
#include <string.h>
#include "MemoryCache.h"
#include "debug.h"

MemoryCache::MemoryCache(size_t size)
	:	fBuffer(0),
		fBufferAreaID(B_BAD_VALUE),
		fReadBytesWanted(0),
		fDataAvailable(B_BAD_SEM_ID),
		fWaitingToWriteBytes(false),
		fBufferSpaceAvailable(B_BAD_SEM_ID),
		fWriteStreamOffset(0),
		fReadStreamOffset(0),
		fAmountRead(0)
{
	fBufferSize = ((size + B_PAGE_SIZE - 1) / B_PAGE_SIZE) * B_PAGE_SIZE;
	fBufferAreaID = create_area("stream_buffer", (void**) &fBuffer,
		B_ANY_ADDRESS, fBufferSize, B_NO_LOCK, B_READ_AREA | B_WRITE_AREA);
	fDataAvailable = create_sem(0, "data available");
	fBufferSpaceAvailable = create_sem(0, "buffer space available");
}

MemoryCache::~MemoryCache()
{
	delete_sem(fDataAvailable);
	delete_sem(fBufferSpaceAvailable);
	delete_area(fBufferAreaID);
}

void MemoryCache::Unset()
{
	delete_sem(fDataAvailable);
	fDataAvailable = -1;
	delete_sem(fBufferSpaceAvailable);
	fBufferSpaceAvailable = -1;
}

ssize_t MemoryCache::Write(const void *data, size_t bytesToCopy)
{
	size_t bytesCopied = 0;
	fBufferLock.Lock();
	while (bytesCopied < bytesToCopy) {
		int32 bytesAvailable = MIN(fAmountRead - (fWriteStreamOffset - fBufferSize),
			fBufferSize - (fWriteStreamOffset % fBufferSize));
		if (bytesAvailable <= 0) {
			// There is no buffer space, wait until some data is read
			fWaitingToWriteBytes = true;
			fBufferLock.Unlock();
			if (acquire_sem_etc(fBufferSpaceAvailable, 1, B_CAN_INTERRUPT, B_INFINITE_TIMEOUT))
				return B_ERROR;
				
			fBufferLock.Lock();
			continue;
		}

		size_t copySize = MIN((size_t) bytesAvailable, bytesToCopy - bytesCopied);
		memcpy(fBuffer + (fWriteStreamOffset % fBufferSize), (char*) data + bytesCopied,
			copySize);
		fWriteStreamOffset += copySize;
		bytesCopied += copySize;

		// Wake the reader that may be waiting.
		if (fReadBytesWanted != 0
			&& fWriteStreamOffset - fAmountRead > fReadBytesWanted) {
			fReadBytesWanted = 0;
			release_sem(fDataAvailable);
		}
	}

	fBufferLock.Unlock();
	return bytesCopied;
}

ssize_t MemoryCache::ReadAt(off_t pos, void *data, size_t bytesToRead)
{
	if (pos < fWriteStreamOffset - fBufferSize)
		return B_ERROR;	// Reading past beginning of buffered window

	fBufferLock.Lock();
	size_t bytesRead = 0;
	while (bytesRead < bytesToRead) {
		int32 bytesAvailableToRead = fWriteStreamOffset - pos;
	
		// Block if there is no data
		if (bytesAvailableToRead <= 0) {
			fReadBytesWanted = bytesToRead - bytesRead;
			fBufferLock.Unlock();
			if (acquire_sem_etc(fDataAvailable, 1, B_CAN_INTERRUPT, B_INFINITE_TIMEOUT) != B_OK)
				return B_ERROR;
				
			fBufferLock.Lock();
			continue;
		}
	
		size_t readSize = MIN((size_t) bytesAvailableToRead, bytesToRead - bytesRead);
		readSize = MIN(readSize, fBufferSize - (pos % fBufferSize));
		memcpy((char*) data + bytesRead, fBuffer + (pos % fBufferSize), readSize);
		bytesRead += readSize;
		pos += readSize;

		// Wake writers that may be waiting
		if (fWaitingToWriteBytes) {
			fWaitingToWriteBytes = false;
			release_sem(fBufferSpaceAvailable);
		}
	}

	fAmountRead = pos;
	fBufferLock.Unlock();
	return bytesRead;
}

off_t MemoryCache::Seek(off_t, uint32)
{
	return B_ERROR;
}

off_t MemoryCache::Position() const
{
	return (off_t) fReadStreamOffset;
}

size_t MemoryCache::AmountBuffered() const
{
	return fWriteStreamOffset - fAmountRead;
}

ssize_t MemoryCache::WriteAt(off_t, const void*, size_t)
{
	return B_ERROR;
}

size_t MemoryCache::Size() const
{
	return fBufferSize;
}



#ifdef _TEST_

bool debug_enabled = true;

int main()
{
	MemoryCache cache(B_PAGE_SIZE);
	const int bufSize = B_PAGE_SIZE - 1;
	char *daBuffer = (char*) malloc(bufSize);
	char c = 0;
	for (int i = 0; i < bufSize; i++)
		daBuffer[i] = c++;

	char *myBuffer = (char*) malloc(bufSize);	
	for (int i = 0; i < B_PAGE_SIZE * 2; i++) {
		cache.Write(daBuffer, bufSize);
		memset(myBuffer, 0, bufSize);
		cache.ReadAt(i * bufSize, myBuffer, bufSize);
		if (memcmp(daBuffer, myBuffer, bufSize) != 0) {
			printf("FAIL\n");
			break;
		} else
			printf("OK\n");
	}
}


#endif


