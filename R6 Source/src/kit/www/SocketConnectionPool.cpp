#include "SocketConnectionPool.h"
#include "SocketConnection.h"

status_t SocketConnectionPool::CreateConnection(const char *hostname, int port,
	Connection **new_connection)
{
	SocketConnection *con = new SocketConnection;
	
	status_t error = con->Open(hostname, port);
	if (error >= B_OK) {
		*new_connection = con;
	} else {
		delete con;
	}
	
	return error;
}
