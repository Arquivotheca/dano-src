#include <support2/Autolock.h>
#include <support2/Debug.h>
#include <www2/DNSCache.h>
#include <www2/ResourceCache.h>
#include <www2/SocketConnection.h>

#include <errno.h>
#include <net/if.h>
#include <net/net_control.h>
#include <netinet/in.h>
#include <sys/ioctl.h>
#include <sys/select.h>
#include <sys/socket.h>

//#include <WagnerDebug.h>

using namespace B::WWW2;

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

void SocketConnection::Close()
{
	// This lock is important.  Sockets fds get recycled, so we don't
	// want to close someone else's socket.	
	BAutolock _lock(fLock.Lock());	
	if (fSocket >= 0) {
		//for some reason, just close()ing a tcp socket doesn't wake up
		//threads currently blocked trying to read/write the socket -
		//calling shutdown() explicitly, however, does
		shutdown(fSocket, SHUTDOWN_BOTH);

 		close(fSocket);
		fSocket = ENOTCONN;
	}
}

status_t SocketConnection::OpenRawSocket(const char *host, int port)
{
	sockaddr_in addr;
 	struct timeval tv;
	int len;
 
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

 	//set the socket's send and receive timeouts to 150 seconds
 	tv.tv_sec = 150;
 	tv.tv_usec = 0;
	setsockopt(fSocket, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
 	setsockopt(fSocket, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

 	if (connect(fSocket, reinterpret_cast<sockaddr*>(&addr), sizeof(addr)) < 0)
		return errno;

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
		
	ssize_t retval = B_ERROR;
	retval = read(fSocket, dest_buffer, count);
	if (retval < 0)
		retval = errno;

	return retval;
}

ssize_t SocketConnection::Write(const void *source_buffer, size_t count)
{
	ASSERT(fSocket >= 0);
	if (fSocket < 0)
		return ENOTCONN;

	size_t sent = 0;
	while (sent < count) {
		ssize_t result = B_ERROR;
	
		result = write(fSocket, source_buffer, count);
		if (result < 0)
			result = errno;
		else if (result == 0)
			result = B_ERROR;	// a zero byte write signifies the end of the stream
	
		if (result < 0)
			return result;
		sent += result;
	}
	
	return count;
}


bool SocketConnection::HasUnreadData()
{
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

ssize_t	SocketConnection::ReadV(const iovec *vector, ssize_t count)
{
#warning "Implement BufferedConnection::ReadV"
}

ssize_t	SocketConnection::WriteV(const iovec *vector, ssize_t count)
{
#warning "Implement BufferedConnection::WriteV"
}

status_t SocketConnection::End()
{
#warning "Implement BufferedConnection::End"
}

status_t SocketConnection::Sync()
{
#warning "Implement BufferedConnection::Sync"
}
