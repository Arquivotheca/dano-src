#ifndef _SOCKET_CONNECTION_H
#define _SOCKET_CONNECTION_H

#include <Gehnaphore.h>
#include "Connection.h"

class SocketConnection : public Connection {
public:

	SocketConnection();
	~SocketConnection();

	// Inherited:
	virtual	ssize_t	Read(void *dest_buffer, size_t length);
	virtual	ssize_t	Write(const void *source_buffer, size_t length);
	virtual void Abort();
	virtual bool HasUnreadData();
	
	// Additional interface:
	virtual status_t Open(const char *host, int port);
	virtual void Close();
	inline int Socket() const;

private:

	status_t OpenRawSocket(const char *host, int port);

	mutable Gehnaphore fLock;
	volatile int fSocket;
	uint32 fLocalAddr, fRemoteAddr;
};

inline int SocketConnection::Socket() const
{
	return fSocket;
}

#endif // _SOCKET_CONNECTION_H
