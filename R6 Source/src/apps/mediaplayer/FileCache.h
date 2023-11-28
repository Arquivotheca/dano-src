#ifndef _CHUNK_CACHE_H
#define _CHUNK_CACHE_H

#include <File.h>
#include <DataIO.h>

class FileCache : public BPositionIO {
public:

	FileCache();
	~FileCache();

	void SetMaxSize(size_t size);
	status_t InitCheck() const;
	status_t Reset();
	virtual	ssize_t ReadAt(off_t pos, void *buffer, size_t size);
	virtual	ssize_t WriteAt(off_t pos, const void *buffer, size_t size);
	virtual off_t Seek(off_t position, uint32 seek_mode);
	virtual	off_t Position() const;
	off_t FindNextFreeChunk(off_t fromOffset) const;
	bool IsDataAvailable(off_t offset, size_t size) const;

	void DumpChunkList() const;
	bool SanityCheck();

private:

	void Cleanup();
	
	off_t fStreamOffset;
	BFile fCacheFile;
	status_t fError;
	struct EmptyChunk {
		off_t fStartOffset;
		off_t fEndOffset;
		EmptyChunk *fNext;
		EmptyChunk *fPrevious;
	} fChunkListHead;
};


#endif
