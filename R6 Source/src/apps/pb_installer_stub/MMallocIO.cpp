#include "MMallocIO.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

MMallocIO::MMallocIO(char *data, size_t size)
{
	fBlockSize = 4096;
	
	fPtr = data;
	fPhysSize = size;
	fLogSize = size;
	fDispose = false;
	if (!fPtr) {
		if (!size) {
			fPtr = malloc(fBlockSize);	//	make sure we never get NULL
			fPhysSize = fBlockSize;
			fLogSize = 0;
		}
		else
			fPtr = malloc(size);
		fDispose = true;
	}
	fPosition = 0;
}


MMallocIO::~MMallocIO()
{
	if (fDispose && fPtr) free(fPtr);
}


ssize_t MMallocIO::ReadAt(
	off_t pos,
	void * buffer, 
	size_t size)
{
	if (pos >= fLogSize) {
		return 0;
	}
	// don't read past end
	if (size > fLogSize-pos)
		size = fLogSize-pos;
	memcpy(buffer, ((char *)fPtr)+pos, size);
	// fLogPos += size;
	return size;
}


ssize_t MMallocIO::WriteAt(
	off_t			pos,
	const void *	buffer,
	size_t			size)
{
	if (size + pos > fPhysSize) {
		// need more blocks
		ssize_t	needed = (size + pos);
		// extra amount exceeding block size
		size_t	block = (needed % fBlockSize);
		if ( block)
			block = fBlockSize - block;
		ssize_t newSize = needed + block;
		
		void *newPtr = realloc(fPtr, newSize);
		if (!newPtr)
			return B_NO_MEMORY;
		fPtr = newPtr;
		fPhysSize = newSize;
	}
	memcpy(((char *)fPtr)+pos, buffer, size);
	
	fPosition = pos + size;
	
	if (fPosition > fLogSize) {
		fLogSize = fPosition;	
	}
	return size;
}


off_t MMallocIO::Seek(
	off_t position,
	uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			break;
		case SEEK_CUR:
			position = fPosition + position;
			break;
		case SEEK_END:
			position = fLogSize - position;
			break;
		default:
			return B_ERROR;
	}
	if (position < 0 || position > fLogSize)
		return B_ERROR;
	fPosition = position;
	
	return B_NO_ERROR;
}


off_t MMallocIO::Position() const
{
	return fPosition;
}

void	MMallocIO::SetBlockSize(size_t blocksize)
{
	if (blocksize > 0)
		fBlockSize = blocksize;
}

void	*MMallocIO::Buffer() const
{
	return fPtr;
}

void	MMallocIO::SetDispose(bool dispose)
{
	fDispose = dispose;
}

off_t MMallocIO::Size() const
{
	return fLogSize;
}


/**
ssize_t MMallocIO::SetSize(off_t size)
{
	if (fLogSize != size) {
		if (size > fPhysSize-fOffset) {
			void *newPtr = realloc(fPtr, size+fOffset);
			if (!newPtr)
				return B_NO_MEMORY;
			fPtr = newPtr;
			fPhysSize = size+fOffset;
		}
		fLogSize = size;
		if (fLogPos > size)
			fLogPos = size;
	}
	return B_NO_ERROR;
}
***/
