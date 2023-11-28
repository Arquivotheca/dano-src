#ifndef _SOCKET_CONNECTION_POOL_H
#define _SOCKET_CONNECTION_POOL_H

#include "ConnectionPool.h"

class SocketConnectionPool : public ConnectionPool {
public:
	virtual status_t CreateConnection(const char *hostname, int port,
		Connection **new_connection);
};

#endif // _SOCKET_CONNECTION_POOL_H
