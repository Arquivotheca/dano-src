#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <Debug.h>
#include <errno.h>
#include "TCPSocket.h"
#include "debug.h"

//
//	Right now, buffered reading has an off by one error that drops a byte
//	of data periodically.
//
#define BUFFERED_READS 0

TCPSocket::TCPSocket()
	:	fBufferSize(0),
		fBufferPos(0),
		fIsOpen(false),
		fSocketFD(-1)
{
	fBuffer = new char[kMinimumReadSize];
}

TCPSocket::~TCPSocket()
{
	delete [] fBuffer;
}

status_t TCPSocket::Open(const char *hostname, int port)
{
	fHostName = hostname;
	fPort = port;	
	return Reopen();
}

bool TCPSocket::IsOpen()
{
	if (!fIsOpen)
		return false;
		
	return true;
}

status_t TCPSocket::Reopen()
{
	if (fSocketFD > 0)
		Close();

	hostent *entry = gethostbyname(fHostName.String());
	if (entry == 0)
		return h_errno;

	fSocketFD = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fSocketFD < 0)
		return errno;
		
	sockaddr_in addr;
	addr.sin_family = AF_INET;
	addr.sin_port = htons(fPort);
	addr.sin_addr.s_addr = *((long*) entry->h_addr);
		
	if (connect(fSocketFD, (struct sockaddr*) &addr, sizeof(sockaddr_in)) < 0) {
		close(fSocketFD);
		return errno;
	}

	return B_OK;
}

void TCPSocket::Close()
{
	if (fSocketFD > 0)
		close(fSocketFD);
}

ssize_t TCPSocket::Read(void *buffer, size_t size)
{
	ASSERT(size != 0);

#if BUFFERED_READS
	// Copy out of the buffer
	if (fBufferPos < fBufferSize) {
		ssize_t sizeToCopy = MIN(size, (size_t)(fBufferSize - fBufferPos));
		ASSERT(sizeToCopy > (ssize_t) 0);
		memcpy(buffer, (char*) fBuffer + fBufferPos, sizeToCopy);
		fBufferPos += sizeToCopy;
		ASSERT(sizeToCopy != 0);
		return sizeToCopy;
	}

	if (size < (size_t) kMinimumReadSize) {
		// Small read, use the buffer
		ASSERT(fBufferPos >= fBufferSize);
		ssize_t sizeReceived;
		sizeReceived = recv(fSocketFD, (char*) fBuffer, kMinimumReadSize, 0);
		if (sizeReceived <= 0) {
			if (errno != EINTR)
				fIsOpen = false;

			return errno;
		}
		
		fBufferSize = sizeReceived;
		ssize_t sizeToCopy = MIN(size, (size_t) fBufferSize);
		memcpy(buffer, fBuffer, sizeToCopy);
		fBufferPos = sizeToCopy;
		ASSERT(sizeToCopy != 0);
		return sizeToCopy;
	} else {
		//	Large read, read directly from the socket
		ssize_t sizeReceived;
		sizeReceived = recv(fSocketFD, (char*) buffer, size, 0);
		if (sizeReceived <= 0) {
			if (errno != EINTR)
				fIsOpen = false;
	
			return errno;
		}

		return sizeReceived;	
	}
	
	TRESPASS();
	return 0;
#else
	ssize_t sizeReceived;
	sizeReceived = recv(fSocketFD, (char*) buffer, size, 0);
	if (sizeReceived <= 0) {
		if (errno != EINTR)
			fIsOpen = false;

		return errno;
	}

	return sizeReceived;	
#endif
}

ssize_t TCPSocket::Write(const void *buffer, size_t size)
{
	size_t sent = 0;
	while (sent < size) {
		ssize_t result = send(fSocketFD, (char*) buffer + sent, size - sent, 0);
		if (result < 0) {
			if (errno != EINTR)
				fIsOpen = false;

			return result;
		}
		
		sent -= result;
	}

	return size;
}

ssize_t TCPSocket::WriteLine(const char *fmt, ...)
{
   	va_list ap; 
	char buf[4096];

	va_start(ap, fmt); 
	vsprintf(buf, fmt, ap);
	va_end(ap); 		

	ASSERT(strlen(buf) < 4096);
	return Write(buf, strlen(buf));
}
