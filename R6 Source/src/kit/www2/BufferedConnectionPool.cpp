#include <www2/BufferedConnection.h>
#include <www2/BufferedConnectionPool.h>

using namespace B::WWW2;

status_t BufferedConnectionPool::CreateConnection(const char *hostname, int port,
	Connection **new_connection)
{
	BufferedConnection *con = new BufferedConnection;
	
	status_t error = con->Open(hostname, port);
	if (error >= B_OK) {
		*new_connection = con;
	} else {
		delete con;
	}
	
	return error;
}
