#include "SSLConnectionPool.h"
#include "SSLConnection.h"

status_t SSLConnectionPool::CreateConnection(const char *hostname, int port,
	Connection **new_connection)
{
	SSLConnection *con = new SSLConnection;
	
	status_t error = con->Open(hostname, port);
	if (error >= B_OK) {
		*new_connection = con;
	} else {
		delete con;
	}
	
	return error;
}
