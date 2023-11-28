#ifndef _TCP_SOCKET_H
#define _TCP_SOCKET_H

#include <DataIO.h>
#include <String.h>

const int32 kMinimumReadSize = 0x8000;

class TCPSocket : public BDataIO {
public:

	TCPSocket();
	~TCPSocket();

	status_t Open(const char *hostname, int port);
	bool IsOpen();
	status_t Reopen();
	void Close();
	
	virtual	ssize_t	Read(void *buffer, size_t size);
	virtual	ssize_t	Write(const void *buffer, size_t size);
	ssize_t WriteLine(const char *format, ...);

private:

	char *fBuffer;
	int32 fBufferSize;
	int32 fBufferPos;

	BString fHostName;
	int fPort;
	bool fIsOpen;

	int fSocketFD;
};


#endif
