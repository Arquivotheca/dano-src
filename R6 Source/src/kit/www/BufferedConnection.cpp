#include <Debug.h>
#include <WagnerDebug.h>
#include "BufferedConnection.h"


BufferedConnection::BufferedConnection()
	:	fAmountBuffered(0),
		fBufferOffset(0)
{
}

BufferedConnection::~BufferedConnection()
{
	Close();
}

void BufferedConnection::Close()
{
	fAmountBuffered = 0;
	fBufferOffset = 0;
	SocketConnection::Close();
}

ssize_t BufferedConnection::UnbufferedRead(void *dest_buffer, size_t count)
{
	return SocketConnection::Read(dest_buffer, count);
}

ssize_t BufferedConnection::UnbufferedWrite(const void *source_buffer, size_t count)
{
	return SocketConnection::Write(source_buffer, count);
}

ssize_t BufferedConnection::Read(void *dest_buffer, size_t count)
{
	//	If there is buffered data, copy that first
	if (fBufferOffset < fAmountBuffered) {
		size_t sizeToCopy = MIN(count, fAmountBuffered - fBufferOffset);
		memcpy(dest_buffer, fBuffer + fBufferOffset, sizeToCopy);
		fBufferOffset += sizeToCopy;
		return sizeToCopy;
	}

	ASSERT(fBufferOffset == fAmountBuffered);

	if (count < kLargeReadThreshold) {
		//	This is a small read.  It would be inefficient to do a lot
		// 	of these, so read into a larger buffer and copy smaller chunks
		// 	out of that.
		ssize_t sizeReceived = UnbufferedRead(fBuffer, kLargeReadSize);
		if (sizeReceived <= 0)
			return sizeReceived;
		
		fAmountBuffered = sizeReceived;
		ssize_t sizeToCopy = MIN(count, fAmountBuffered);
		memcpy(dest_buffer, fBuffer, sizeToCopy);
		fBufferOffset = sizeToCopy;
		return sizeToCopy;
	}

	//	This is a fairly large read, so it just wastes time to
	//	copy into a temporary buffer.  Read from the socket directly into
	//	the user buffer
	return UnbufferedRead(dest_buffer, count);
}

ssize_t BufferedConnection::Write(const void *source_buffer, size_t count)
{
	return UnbufferedWrite(source_buffer, count);
}

