/*
	BufferedFileAdapter.cpp
*/
#include <OS.h>
#include "BufferedFileAdapter.h"

BufferedFileAdapter::BufferedFileAdapter(BDataIO *source, int bufferSize, bool owning)
	:	fSource(source),
		fOwning(owning),
		fBufferPosition(0),
		fBytesInBuffer(0),
		fBufferSize(bufferSize),
		fBuffer(NULL)
{
	fBuffer = new char[fBufferSize];
}

BufferedFileAdapter::~BufferedFileAdapter()
{
	delete [] fBuffer;
	if (fOwning)
		delete fSource;
}

inline ssize_t BufferedFileAdapter::FillBuffer()
{
	fBufferPosition = 0;
	fBytesInBuffer = fSource->Read(fBuffer, fBufferSize);
	return fBytesInBuffer;
}

ssize_t BufferedFileAdapter::Read(void *buffer, size_t size)
{
	// This adapter is a little different. It tries to copy
	// as much data into the buffer as was requested. This
	// way, the caller doesn't have to keep calling back in.
	// This class is pretty much only used when the amount
	// that you want to read is already known. (Like when
	// reading fixed sized blocks of data out of a file.)
	size_t copied = 0;
	char *ptr = static_cast<char *>(buffer);
	while (copied < size) {
		if (fBufferPosition == fBytesInBuffer)
			if (FillBuffer() == 0)
				return copied;
		ssize_t sizeToRead = MIN((ssize_t)(size - copied), (ssize_t)(fBytesInBuffer - fBufferPosition));
		memcpy(ptr + copied, &fBuffer[fBufferPosition], sizeToRead);
		fBufferPosition += sizeToRead;
		copied += sizeToRead;		
	}
	return copied;
}

char BufferedFileAdapter::Peek()
{
	if (fBufferPosition == fBytesInBuffer)
		FillBuffer();
	return fBuffer[fBufferPosition];
}

ssize_t BufferedFileAdapter::Write(const void *, size_t )
{
	return B_ERROR;
}
