#ifndef _BUFFERED_CONNECTION_H
#define _BUFFERED_CONNECTION_H

#include <www2/SocketConnection.h>

namespace B {
namespace WWW2 {

// Note that kLargeReadThreshold is smaller than the size of ResourceCache reads.
// It needs to be (otherwise the data would always be copied).
const size_t kLargeReadThreshold = 1024;
const int kLargeReadSize = 0x3000;

class BufferedConnection : public SocketConnection
{
	public:
	
		BufferedConnection();
		~BufferedConnection();
	
		// Inherited:
		virtual	ssize_t	Read(void *dest_buffer, size_t length);
		virtual	ssize_t ReadV(const iovec *vector, ssize_t count);
		virtual	ssize_t	Write(const void *source_buffer, size_t length);
		virtual	ssize_t	WriteV(const iovec *vector, ssize_t count);
		virtual status_t End();
		virtual	status_t Sync();
		
		virtual void Close();
	
		// Additional interface:
		virtual ssize_t UnbufferedRead(void *dest_buffer, size_t length);
		virtual ssize_t UnbufferedWrite(const void *source_buffer, size_t length);
	
	private:
	
		char fBuffer[kLargeReadSize];
		size_t fAmountBuffered;
		size_t fBufferOffset;

};

} } // namespace B::WWW2

#endif // _BUFFERED_CONNECTION_H
