// ===========================================================================
//	NetPositiveStreamIO.cpp
//  Copyright 1999 by Be Incorporated.
// ===========================================================================

#ifdef PLUGINS

#include "NetPositiveStreamIO.h"

#include <unistd.h>
#include <malloc.h>
#include <Message.h>
#include <stdio.h>

void BNetPositiveStreamIO::Dereference()
{
	if (--fRefCount == 0)
		delete this;
}


void BNetPositiveStreamIO::Reference()
{
	fRefCount++;
}


ssize_t BNetPositiveStreamIO::ReadAt(off_t pos, void *buffer, size_t size)
{
	return ReadAtWithTimeout(pos, buffer, size, B_INFINITE_TIMEOUT);
}


ssize_t BNetPositiveStreamIO::ReadWithTimeout(void *buf, size_t size, bigtime_t timeout)
{
	ssize_t		res;

	res = ReadAtWithTimeout(Position(), buf, size, timeout);
	if (res >= 0)
		Seek(Position() + res, SEEK_SET);
	return res;
}


ssize_t BNetPositiveStreamIO::ReadAtWithTimeout(off_t pos, void *buffer, size_t size, bigtime_t timeout)
{
	bigtime_t start_time = system_time();
	
//size_t orig_size = size;
	while (pos >= fLength && (timeout == B_INFINITE_TIMEOUT || (timeout != 0 && start_time + timeout > system_time()))) {
//printf("ReadAt  %Ld, requested %ld snoozing\n", pos, orig_size);
		snooze(100000);
	}

	if (pos >= fLength)
		return 0;
		
	if (pos + size > fLength)
		size = fLength - pos;

//printf("ReadAt  %Ld, requested %ld, read  %ld, fPosition is now %Ld\n", pos, orig_size, size, fPosition);
	memcpy(buffer, fData + pos, size);
	return size;
}


ssize_t BNetPositiveStreamIO::WriteAt(off_t pos, const void *buffer, size_t size)
{
	char		*newdata;
	size_t		newsize;
//size_t orig_size = size;

	if (pos + size > fLength) {
		if (pos + size > fMallocSize) {
			newsize = ((pos + size + (fBlockSize-1)) / fBlockSize) * fBlockSize;
			newdata = (char *) realloc(fData, newsize);
			if (!newdata)
				return ENOMEM;
			fData = newdata;
			fMallocSize = newsize;
		}
		fLength = pos + size;
	}

	memcpy(fData + pos, buffer, size);

//printf("WriteAt %Ld, requested %ld, wrote %ld, fPosition is now %Ld\n", pos, orig_size, size, fPosition);
	return size;
}


off_t BNetPositiveStreamIO::Seek(off_t position, uint32 seek_mode)
{
	switch (seek_mode) {
		case SEEK_SET:
			fPosition = position;
			break;
		case SEEK_CUR:
			fPosition = fPosition + position;
			break;
		case SEEK_END:
			fPosition = fLength + position;
			break;
	}
	return fPosition;
}


off_t BNetPositiveStreamIO::Position() const
{
	return fPosition;
}


status_t BNetPositiveStreamIO::SetSize(off_t size)
{
	if (size > fLength) {
		if (size > fMallocSize) {
			char	*newdata;
			size_t	newsize;
			newsize = ((size + (fBlockSize-1)) / fBlockSize) * fBlockSize;
			newdata = (char *) realloc(fData, newsize);
			if (!newdata)
				return ENOMEM;
			fData = newdata;
			fMallocSize = newsize;
		}
		fLength = size;
	} else if (size < fLength) {
		// don't bother reallocing, just shrink the logical size.
		fLength = size;

		// ??? Is this the right thing to do?
		// if (fPosition > fLength)
		//	fPosition = fLength;
		// NO -- hplus
	}
	return B_OK;
}

BNetPositiveStreamIO::BNetPositiveStreamIO()
{
	fData = NULL;
	fBlockSize = 256;
	fLength = 0;
	fMallocSize = 0;
	fPosition = 0;
	fRefCount = 1;
	fContentLength = 0;
	fError = 0;
}


BNetPositiveStreamIO::~BNetPositiveStreamIO()
{
	if (fData)
		free(fData);
}


ssize_t BNetPositiveStreamIO::ContentLength() const
{
	return fContentLength;
}

const char *BNetPositiveStreamIO::ContentType() const
{
	return fContentType.String();
}

void BNetPositiveStreamIO::SetContentLength(ssize_t length)
{
	fContentLength = length;
}

void BNetPositiveStreamIO::SetContentType(const char *type)
{
	fContentType = type;
}

ssize_t BNetPositiveStreamIO::AmountWritten() const
{
	return fLength;
}

void BNetPositiveStreamIO::SetError(uint32 error)
{
	fError = error;
}

uint32 BNetPositiveStreamIO::GetError() const
{
	return fError;
}

void BNetPositiveStreamIO::_ReservedNetPositiveStreamIO1() {}
void BNetPositiveStreamIO::_ReservedNetPositiveStreamIO2() {}
void BNetPositiveStreamIO::_ReservedNetPositiveStreamIO3() {}
void BNetPositiveStreamIO::_ReservedNetPositiveStreamIO4() {}

#endif
