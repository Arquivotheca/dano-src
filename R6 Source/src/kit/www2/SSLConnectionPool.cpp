#include <www2/SSLConnectionPool.h>
#include <www2/SSLConnection.h>

using namespace B::WWW2;

status_t SSLConnectionPool::CreateConnection(const char *hostname, int port, Connection **new_connection)
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
