#ifndef _BUFFERED_CONNECTION_H
#define _BUFFERED_CONNECTION_H

#include "SocketConnection.h"

// Note that kLargeReadThreshold is smaller than the size of ResourceCache reads.
// It needs to be (otherwise the data would always be copied).
const size_t kLargeReadThreshold = 1024;
const int kLargeReadSize = 0x3000;

class BufferedConnection : public SocketConnection {
public:
	
	BufferedConnection();
	~BufferedConnection();
	
	// Inherited:
	virtual	ssize_t	Read(void *dest_buffer, size_t length);
	virtual	ssize_t	Write(const void *source_buffer, size_t length);
	virtual void Close();
	
	// Additional interface:
	virtual ssize_t UnbufferedRead(void *dest_buffer, size_t length);
	virtual ssize_t UnbufferedWrite(const void *source_buffer, size_t length);

private:

	char fBuffer[kLargeReadSize];
	size_t fAmountBuffered;
	size_t fBufferOffset;

};

#endif // _BUFFERED_CONNECTION_H
