#ifndef _BUFFERED_CONNECTION_POOL_H
#define _BUFFERED_CONNECTION_POOL_H

#include "ConnectionPool.h"

class BufferedConnectionPool : public ConnectionPool {
public:
	virtual status_t CreateConnection(const char *hostname, int port,
		Connection **new_connection);
};

#endif // _BUFFERED_CONNECTION_POOL_H
