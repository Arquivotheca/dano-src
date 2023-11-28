#ifndef _SOCKET_CONNECTION_H
#define _SOCKET_CONNECTION_H

#include <support2/Locker.h>
#include <www2/Connection.h>

namespace B {
namespace WWW2 {

class SocketConnection : public Connection
{
	public:

		SocketConnection();
		~SocketConnection();

		// Inherited:
		virtual	ssize_t	Read(void *dest_buffer, size_t length);
		virtual	ssize_t ReadV(const iovec *vector, ssize_t count);
		virtual	ssize_t	Write(const void *source_buffer, size_t length);
		virtual	ssize_t	WriteV(const iovec *vector, ssize_t count);
		virtual status_t End();
		virtual	status_t Sync();

		virtual bool HasUnreadData();

		// Additional interface:
		virtual status_t Open(const char *host, int port);
		virtual void Close();
		inline int Socket() const;

	private:

		status_t OpenRawSocket(const char *host, int port);

		BLocker fLock;
		int fSocket;
		uint32 fLocalAddr, fRemoteAddr;
};

inline int SocketConnection::Socket() const
{
	return fSocket;
}

} } // namespace B::WWW2

#endif // _SOCKET_CONNECTION_H
