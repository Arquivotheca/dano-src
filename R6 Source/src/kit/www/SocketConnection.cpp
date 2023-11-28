#include <Debug.h>
#include <DNSCache.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/select.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <net/net_control.h>
#include <WagnerDebug.h>
#include <ResourceCache.h>
#include "SocketConnection.h"


SocketConnection::SocketConnection()
	:	fSocket(ENOTCONN)
{
}

SocketConnection::~SocketConnection()
{
	Close();
}

status_t SocketConnection::Open(const char *host, int port)
{
	status_t error = B_OK;

	if (fSocket >= 0)
		Close();
		
	error = OpenRawSocket(host, port);
	
	return error;
}

void SocketConnection::Abort()
{
	// This lock is important.  Sockets fds get recycled, so we don't
	// want to close someone else's socket.	
	GehnaphoreAutoLock _lock(fLock);	
	if (fSocket >= 0) {
		int socket = fSocket;
		fSocket = ENOTCONN;

		//for some reason, just close()ing a tcp socket doesn't wake up
		//threads currently blocked trying to read/write the socket -
		//calling shutdown() explicitly, however, does
		shutdown(socket, SHUTDOWN_BOTH);
 		close(socket);
	}
}

void SocketConnection::Close()
{
	//make sure the socket gets closed
	Abort();
}

status_t SocketConnection::OpenRawSocket(const char *host, int port)
{
	sockaddr_in addr;
 	struct timeval tv;
	int len;
 
	GehnaphoreAutoLock _lock(fLock);

 	memset(&addr, 0, sizeof(addr));
 	addr.sin_len = sizeof(addr);
	addr.sin_family = AF_INET;
	addr.sin_port = htons(port);
	status_t error = dnsCache.GetHostByName(host, &(addr.sin_addr.s_addr));
	if (error != 0)
		return error;
	fRemoteAddr = addr.sin_addr.s_addr;

	fSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if (fSocket < 0)
		return errno;

	//it apparently isn't possible to cancel a tcp connect() once it's
	//been kicked of (neither shutdown() nor close() do the trick), but
	//Close() needs to be able to abort this connection quickly - as such,
	//we'll initiate a non-blocking connect(), but then switch the socket
	//back to blocking mode, and the first read()/write() on the socket
	//(operations which can be interrupted by shutdown()) will wait for
	//the connect() to finish, then do their thing...
	if (fcntl(fSocket, F_SETFL, fcntl(fSocket, F_GETFL) | O_NONBLOCK) != 0)
		return errno;

 	if (connect(fSocket, (struct sockaddr *)&addr, sizeof(addr)) != 0 &&
 		errno != EINPROGRESS)
	{
		return errno;
	}

	//return the socket to blocking mode, by setting its send and receive
	//timeouts to 150 seconds each
 	tv.tv_sec = 150;
 	tv.tv_usec = 0;
	setsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
 	setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

	//remember the ip address the socket is bound to (if our ip address
	//changes, this connection will become invalid)
	len = sizeof(addr);
	if (getsockname(fSocket, reinterpret_cast<sockaddr*>(&addr), &len) != 0)
		return errno;
	fLocalAddr = addr.sin_addr.s_addr;

	return B_OK;
}

ssize_t SocketConnection::Read(void *dest_buffer, size_t count)
{
	if (fSocket < 0)
		return ENOTCONN;

	if (count == 0)
		return 0;
		
	//the socket's read timeout is set to 5 seconds to guarantee a
	//quick response to an Abort() - however, we don't actually want
	//a read to fail unless it stalls for 150 seconds, so we'll loop
	ssize_t retval = B_ERROR;
	retval = read(fSocket, dest_buffer, count);
	if (retval < 0)
		retval = errno;

	return retval;
}

ssize_t SocketConnection::Write(const void *source_buffer, size_t count)
{
	ASSERT(fSocket >= 0);

	const char *buf = (const char *)source_buffer;
	size_t sent = 0;
	while (sent < count) {
		if (fSocket < 0)
			return ENOTCONN;

		ssize_t result = B_ERROR;
		result = write(fSocket, &buf[sent], count - sent);
		if (result < 0)
			result = errno;
		else if (result == 0)
			result = B_ERROR;		//this shouldn't happen

		if (result < 0)
			return result;

		sent += result;
	}
	
	return count;
}


bool SocketConnection::HasUnreadData()
{
	GehnaphoreAutoLock _lock(fLock);

	if (fSocket >= 0) {
		fd_set fdSet;
		struct timeval tv;
	
		FD_ZERO(&fdSet);
		FD_SET(fSocket, &fdSet);
	
		tv.tv_sec = 0;
		tv.tv_usec = 0;
	
		if (select(fSocket + 1, &fdSet, NULL, NULL, &tv) == 1) {
//			CONNECTION_TRACE((CONNECTION_STATUS_COLOR
//				"Connection %p HasUnreadData().\n"CONNECTION_NORMAL_COLOR,
//				this));
			return true;
		}
		else {
			return false;
		}
	}
	else {
		return false;
	}
}

