#ifndef _SSL_CONNECTION_POOL_H
#define _SSL_CONNECTION_POOL_H

#include <www2/ConnectionPool.h>

namespace B {
namespace WWW2 {

class SSLConnectionPool : public ConnectionPool
{
	public:
		virtual status_t CreateConnection(const char *hostname, int port, Connection **new_connection);
};

} } // namespace B::WWW2

#endif // _SSL_CONNECTION_POOL_H
