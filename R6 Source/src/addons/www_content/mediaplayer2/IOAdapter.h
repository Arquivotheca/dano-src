//
//	Adapt a BDataIO to a BPositionIO (read only), assuming limited seek back/forward.
//

#ifndef _IO_ADAPTER_H
#define _IO_ADAPTER_H

#include <DataIO.h>

class IOAdapter : public BPositionIO {
public:
	IOAdapter(BDataIO*, int32 size);
	virtual ~IOAdapter();
	virtual	ssize_t ReadAt(off_t, void*, size_t);
	void SetStreamLength(off_t);
	off_t AmountRead() const;
	void Abort();

private:
	static int32 StartReadAhead(void*);
	void ReadAhead();
	inline int32 ReadAheadBytesAvailable() const;
	virtual	ssize_t WriteAt(off_t, const void*, size_t);
	virtual off_t Seek(off_t, uint32 mode);
	virtual	off_t Position() const;

	BDataIO *fSource;
	char *fBuffer;
	int32 fBufferSize;
	int32 fWaitFlags;
	int32 fSourceOffset;
	int32 fStreamLength;
	int32 fAmountRead;
	int32 fSeekPosition;
	sem_id fReadAheadWait;
	sem_id fReadWait;
	thread_id fReadAheadThread;
};

#endif
