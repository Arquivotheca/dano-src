#ifndef _SSL_CONNECTION_POOL_H
#define _SSL_CONNECTION_POOL_H

#include "ConnectionPool.h"

class SSLConnectionPool : public ConnectionPool {
public:
	virtual status_t CreateConnection(const char *hostname, int port,
		Connection **new_connection);
};

#endif // _SSL_CONNECTION_POOL_H
