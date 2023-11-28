#include <Debug.h>
#include <stdio.h>
#include <stdlib.h>
#include "IOAdapter.h"

const int32 kMinReadBack = 0x2000;

enum WaitFlags {
	kQuit = 1,
	kReadAheadWait = 2,
	kReadWait = 4
};

IOAdapter::IOAdapter(BDataIO *source, int32 size)
	:	fSource(source),
		fBufferSize(size),
		fWaitFlags(0),
		fSourceOffset(0),
		fStreamLength(0x7fffffff),
		fAmountRead(0),
		fSeekPosition(0)
{
	ASSERT(size >= kMinReadBack * 2);

	fBuffer = new char[size];
	fReadAheadWait = create_sem(0, "Encoded Audio Buffer Full");
	fReadWait = create_sem(0, "Encoded Audio Buffer Empty");
	fReadAheadThread = spawn_thread(StartReadAhead, "Buffer Raw Data", 17 , this);
	resume_thread(fReadAheadThread);
}

IOAdapter::~IOAdapter()
{
	if ((fWaitFlags & kQuit) == 0)
		Abort();
		
	delete [] fBuffer;
}

void IOAdapter::Abort()
{
	atomic_or(&fWaitFlags, kQuit);
	delete_sem(fReadAheadWait);
	fReadAheadWait = B_BAD_SEM_ID;
	delete_sem(fReadWait);
	fReadWait = B_BAD_SEM_ID;
	status_t err;
	wait_for_thread(fReadAheadThread, &err);
}

inline int32 IOAdapter::ReadAheadBytesAvailable() const
{
	return fBufferSize - (fSourceOffset - fAmountRead) - kMinReadBack;
}

ssize_t IOAdapter::ReadAt(off_t pos, void *buffer, size_t count)
{
	if (pos < 0 || pos < fSourceOffset - fBufferSize)
		return B_ERROR;	// Cannot read before beginning of buffer
	
	if (pos >= fStreamLength)
		return B_ERROR;

	if (pos + count > fStreamLength)
		count = fStreamLength - pos;
	
	int totalRead = 0;
	fAmountRead = pos;
	while ((fWaitFlags & kQuit) == 0 && totalRead < count) {
		if (fAmountRead >= fSourceOffset && fAmountRead < fStreamLength) {
			// Note: the atomic_or handles the lost wakeup case where the
			// bufferer sleeps waits right before this thread releases
			// space in the buffer pool.
			if (atomic_or(&fWaitFlags, kReadWait) & kReadAheadWait) {
				atomic_and(&fWaitFlags, ~kReadAheadWait);
				release_sem(fReadAheadWait);
			}

			if(B_OK!=acquire_sem(fReadWait))
				break;
			continue;
		}

		if (fAmountRead >= fStreamLength)
			break;

		int sizeToCopy = MIN(fSourceOffset - fAmountRead, count - totalRead);	// amount available or amount requested
		sizeToCopy = MIN(sizeToCopy, fBufferSize - (fAmountRead % fBufferSize)); // wraparound
		memcpy(reinterpret_cast<char*>(buffer) + totalRead, fBuffer + (fAmountRead
			% fBufferSize), sizeToCopy);
		totalRead += sizeToCopy;
		fAmountRead += sizeToCopy;
		if (ReadAheadBytesAvailable() > 0 && (fWaitFlags & kReadAheadWait)) {
			atomic_and(&fWaitFlags, ~kReadAheadWait);
			release_sem(fReadAheadWait);
		}
	}

	return totalRead;
}

int32 IOAdapter::StartReadAhead(void *castToIOAdapter)
{
	reinterpret_cast<IOAdapter*>(castToIOAdapter)->ReadAhead();
	return 0;
}

void IOAdapter::ReadAhead()
{
	while ((fWaitFlags & kQuit) == 0 && fSourceOffset < fStreamLength) {
		int32 sizeToCopy = MIN(ReadAheadBytesAvailable(), fBufferSize - fSourceOffset
			% fBufferSize);
		if (sizeToCopy <= 0) {
			if (atomic_or(&fWaitFlags, kReadAheadWait) & kReadWait) {
				atomic_and(&fWaitFlags, ~kReadWait);
				release_sem(fReadWait);		
			}
			
			acquire_sem(fReadAheadWait);
			continue;
		}

		ssize_t sizeCopied = fSource->Read(fBuffer + (fSourceOffset % fBufferSize), sizeToCopy);
		if (sizeCopied <= 0) {
			fStreamLength = fSourceOffset;
			break;
		}

		fSourceOffset += sizeCopied;	
	}
	delete_sem(fReadWait);
	fReadWait = B_BAD_SEM_ID;
}

void IOAdapter::SetStreamLength(off_t len)
{
	fStreamLength = len;
}

ssize_t IOAdapter::WriteAt(off_t, const void*, size_t)
{
	return B_ERROR;
}

off_t IOAdapter::Seek(off_t offset, uint32 mode)
{
	switch (mode) {
		case SEEK_SET:
			fSeekPosition = offset;
			break;
			
		case SEEK_CUR:
			fSeekPosition += offset;
			break;
			
		case SEEK_END:
			fSeekPosition = fStreamLength + offset;
			break;
	
		default:
			return B_ERROR;
	}

	return fSeekPosition;
}

off_t IOAdapter::Position() const
{
	return fSeekPosition;
}

off_t IOAdapter::AmountRead() const
{
	return fAmountRead;
}


#if TEST_IO_ADAPTER

#include <signal.h>

int main()
{
	printf("testing...\n");
	const char *kTestString = "test string  ";
	int len = strlen(kTestString);
	const int kReps = 76512;
	BMallocIO source;
	for (int i = 0; i < kReps; i++)
		source.Write(kTestString, len);

	source.Seek(0, SEEK_SET);
	IOAdapter io(&source, 0x10000);
	int cur = 0;
	off_t off = 0;
	for (;;) {
		char buf[1024];
		ssize_t amountRead = io.ReadAt(off, buf, (unsigned) rand() % 1024);
		if (amountRead < 0)
			break;
			
		off += amountRead;
		for (int i = 0; i < amountRead; i++) {
			if (buf[i] != kTestString[cur]) {
				printf("FAILED: data mismatch at offset %Ld\n", off + i);
				return 1;
			}
			
			cur = (cur + 1) % len;
		}
	}

	if (off != kReps * len)
		printf("FAILED: file truncated\n");
	else
		printf("PASSED\n");
}

#endif

