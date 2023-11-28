#include "Pusher.h"


Pusher::Pusher(void)
	: fSink(0)
{
}


Pusher::Pusher(Pusher *p)
	: fSink(p)
{
}


Pusher::~Pusher()
{
	delete fSink;
}

ssize_t 
Pusher::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	return fSink->Write(buffer, length, finish);
}

#if 0
status_t 
Pusher::Flush(void)
{
	return fSink->Flush();
}

#endif

PusherBuffer::PusherBuffer(Pusher *sink, ssize_t bufferSize)
	: Pusher(sink), fBuffer(new uint8[bufferSize]), fMaxSize(bufferSize), fBufSize(0), fNextByte(0)
{
}


PusherBuffer::~PusherBuffer()
{
	delete [] fBuffer;
}

ssize_t 
PusherBuffer::Write(const uint8 *buffer, ssize_t length, bool finish)
{
	ssize_t result;
	ssize_t origLength = length;
	ssize_t bytesToCopy;

refill_buffer:
	// if buffer had contents
	if (fNextByte > 0)
	{
		// if bytes remained
		if (fNextByte < fBufSize)
		{
			// how many bytes to shuffle
			bytesToCopy = fBufSize - fNextByte;
			// shuffle the bytes to the begining of the buffer
			memcpy(fBuffer, fBuffer + fNextByte, bytesToCopy);
			// hold on to this spot
			fBufSize = bytesToCopy;
		}
		// otherwise
		else
		{
			// start from the begining
			fBufSize = 0;
		}
	}
	// first available byte from the front of the buffer
	fNextByte = 0;
	// determine how many bytes of the source buffer to copy
	bytesToCopy = fMaxSize - fBufSize;
	if (length < bytesToCopy) bytesToCopy = length;
	// copy bytes from input buffer
	memcpy(fBuffer + fBufSize, buffer, bytesToCopy);
	// adjust number of input bytes
	length -= bytesToCopy;
	// adjust begining of input bytes
	buffer += bytesToCopy;
	// adjust number of bytes in buffer
	fBufSize += bytesToCopy;
	// avoid writing small chunks
	if (!finish && (length == 0) && (fBufSize < fMaxSize)) return origLength;

	// write the data to our sink
	do
	{
		// push the bytes down stream
		result = Pusher::Write(fBuffer + fNextByte, fBufSize - fNextByte, finish && (length == 0));
		// if we got a good write
		if (result > 0)
		{
			// advance pointer
			fNextByte += result;
		}
	}
	while ((result > 0) && (fNextByte < fBufSize));
	// if we got a good write
	if (result >= 0)
	{
		// refill if more bytes left in source buffer
		if (length != 0) goto refill_buffer;
		// otherwise note the proper number of bytes written
		result = origLength;
	}
	// reset condition?
	if (finish) fBufSize = fNextByte = 0;
	return result;
}

