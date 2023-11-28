#ifndef _SOCKET_CONNECTION_POOL_H
#define _SOCKET_CONNECTION_POOL_H

#include <www2/ConnectionPool.h>

namespace B {
namespace WWW2 {

class SocketConnectionPool : public ConnectionPool
{
	public:
		virtual status_t CreateConnection(const char *hostname, int port, Connection **new_connection);
};

} } // namespace B::WWW2

#endif // _SOCKET_CONNECTION_POOL_H
