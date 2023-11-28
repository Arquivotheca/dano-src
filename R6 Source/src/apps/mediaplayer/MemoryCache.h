#ifndef _BUFFERED_STREAM_H
#define _BUFFERED_STREAM_H

#include <OS.h>
#include <Locker.h>
#include <DataIO.h>
#include <Debug.h>
#include <Autolock.h>

class MemoryCache : public BPositionIO {
public:

	MemoryCache(size_t size = 0x30000);
	~MemoryCache();	
	void Unset();
	virtual	ssize_t Write(const void*, size_t);
	virtual	ssize_t ReadAt(off_t, void*, size_t);
	virtual off_t Seek(off_t, uint32);
	virtual	off_t Position() const;
	void SetBuffering(size_t highWater);
	size_t AmountBuffered() const;
	size_t Size() const;

private:
	virtual	ssize_t WriteAt(off_t, const void*, size_t);

	mutable BLocker fBufferLock;
	char *fBuffer;
	area_id fBufferAreaID;
	size_t fBufferSize;

	size_t fReadBytesWanted;
	sem_id fDataAvailable;
	bool fWaitingToWriteBytes;
	sem_id fBufferSpaceAvailable;
	off_t fWriteStreamOffset;
	off_t fReadStreamOffset;
	off_t fAmountRead;
};

#endif
