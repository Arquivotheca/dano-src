#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <Autolock.h>
#include <Debug.h>
#include <Entry.h>
#include "FileCache.h"

FileCache::FileCache()
	:	fError(-1)
{
	fChunkListHead.fNext = &fChunkListHead;
	fChunkListHead.fPrevious = &fChunkListHead;
	Reset();
}

FileCache::~FileCache()
{
	if (fError >= 0)
		Cleanup();
}

status_t FileCache::Reset()
{
	if (fError >= 0)
		Cleanup();

	// Start the free chunk list with a single really large chunk
	EmptyChunk *totalSpace = new EmptyChunk;
	totalSpace->fStartOffset = 0;
	totalSpace->fEndOffset = 0x7fffffffffffffffLL;
	fChunkListHead.fNext = totalSpace;
	fChunkListHead.fPrevious = totalSpace;
	totalSpace->fNext = &fChunkListHead;
	totalSpace->fPrevious = &fChunkListHead;

	// Open a backing file
	int serialNumber = 0;
	char fileName[64];
	while (true) {
		sprintf(fileName, "/tmp/media_download_%d", serialNumber++);
		fError = fCacheFile.SetTo(fileName, O_RDWR | O_CREAT | O_EXCL);
		if (fError == B_FILE_EXISTS)
			continue;
	
		break;
	}

	BEntry(fileName).Remove();
	return fError;
}

void FileCache::Cleanup()
{
	while (fChunkListHead.fNext != &fChunkListHead) {
		EmptyChunk *next = fChunkListHead.fNext;
		fChunkListHead.fNext = fChunkListHead.fNext->fNext;
		delete next;
	}

	fCacheFile.Unset();
	fError = -1;
}

status_t FileCache::InitCheck() const
{
	return fError;
}

ssize_t FileCache::ReadAt(off_t offset, void *data, size_t size)
{
	ASSERT(offset >= 0);
	ASSERT(size > 0);
	ASSERT(IsDataAvailable(offset, size));
	ASSERT(fError >= 0);

	return fCacheFile.ReadAt(offset, data, size);
}


status_t FileCache::WriteAt(off_t offset, const void *data, size_t size)
{
	ASSERT(offset >= 0);
	ASSERT(size > 0);
	ASSERT(data != 0);
	if (fError < 0)
		return fError;

	// Update the free chunk list
	for (EmptyChunk *chunk = fChunkListHead.fNext; chunk != &fChunkListHead
		&& chunk->fStartOffset < offset + size; chunk = chunk->fNext) {
		ASSERT(chunk->fStartOffset <= chunk->fEndOffset);
		if (offset <= chunk->fStartOffset && offset + size >= chunk->fEndOffset) {
			// 1. This free chunk is entirely consumed
			chunk->fNext->fPrevious = chunk->fPrevious;
			chunk->fPrevious->fNext = chunk->fNext;
			EmptyChunk *temp = chunk;
			chunk = chunk->fPrevious;	// At the next pass through the loop,
										// this will go the next one
			delete temp;
		} else if (offset > chunk->fStartOffset && offset + size < chunk->fEndOffset) {
			// 2. Split this free chunk into two
			EmptyChunk *newChunk = new EmptyChunk;
			newChunk->fStartOffset = offset + size;
			newChunk->fEndOffset = chunk->fEndOffset;
			chunk->fEndOffset = offset - 1;
			
			newChunk->fNext = chunk->fNext;
			newChunk->fPrevious = chunk;
			newChunk->fNext->fPrevious = newChunk;
			newChunk->fPrevious->fNext = newChunk;
			break;
		} else if (offset + size > chunk->fStartOffset
			&& offset + size < chunk->fEndOffset) {
			// 3. Overlap beginning of free chunk
			chunk->fStartOffset = offset + size;
			break;
		} else if (offset > chunk->fStartOffset && offset < chunk->fEndOffset) {
			// 4. Overlap end of free chunk
			chunk->fEndOffset = offset - 1;
		}
	}

	return fCacheFile.WriteAt(offset, data, size); 
}

off_t FileCache::FindNextFreeChunk(off_t fromOffset) const
{
	ASSERT(fromOffset >= 0);
	ASSERT(fError >= 0);

	// Search from the current position, looking for the first chunk that
	// needs to be read
	for (EmptyChunk *chunk = fChunkListHead.fNext; chunk != &fChunkListHead;
		chunk = chunk->fNext) {
		if (chunk->fStartOffset <= fromOffset && chunk->fEndOffset > fromOffset)
			return MAX(chunk->fStartOffset, fromOffset);
	}

	// Everything is read after this point, loop back and search from
	// the beginning
	if (fChunkListHead.fNext != &fChunkListHead)
		return fChunkListHead.fNext->fStartOffset;

	// No more to read.
	return -1;
}


off_t FileCache::Seek(off_t position, uint32 seek_mode)
{
	if (seek_mode == SEEK_SET)
		fStreamOffset = position;
	else if (seek_mode == SEEK_CUR)
		fStreamOffset += position;

	return -1;
}

off_t FileCache::Position() const
{
	return fStreamOffset;
}

bool FileCache::IsDataAvailable(off_t offset, size_t size) const
{
	ASSERT(offset >= 0);
	ASSERT(size > 0);
	
	for (EmptyChunk *chunk = fChunkListHead.fNext; chunk != &fChunkListHead
		&& chunk->fStartOffset < offset + size; chunk = chunk->fNext) {
		ASSERT(chunk->fStartOffset <= chunk->fEndOffset);
		if (MAX(offset, chunk->fStartOffset) <= MIN(offset + size - 1, chunk->fEndOffset))
			return false;
	}

	return true;
}

bool FileCache::SanityCheck()
{
	for (EmptyChunk *chunk = fChunkListHead.fNext; chunk != &fChunkListHead;
		chunk = chunk->fNext) {
		
		// crossed pointers
		if (chunk->fStartOffset > chunk->fEndOffset)
			return false;

		// bad list	
		if (chunk->fNext->fPrevious != chunk)
			return false;
			
		if (chunk->fPrevious->fNext != chunk)
			return false;

		// overlapped chunks			
		if (chunk->fNext != &fChunkListHead &&
			chunk->fNext->fStartOffset < chunk->fEndOffset)
			return false;
	}

	return true;
}

void FileCache::SetMaxSize(size_t size)
{
	// This only works if there is one free chunk in the list.
	ASSERT(fChunkListHead.fNext != &fChunkListHead);
	ASSERT(fChunkListHead.fNext->fNext == &fChunkListHead);

	fChunkListHead.fNext->fEndOffset = size;
}

void FileCache::DumpChunkList() const
{
	for (EmptyChunk *chunk = fChunkListHead.fNext; chunk != &fChunkListHead;
		chunk = chunk->fNext)
		printf("%Li - %Li\n", chunk->fStartOffset, chunk->fEndOffset);		
}
