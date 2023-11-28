#ifndef _FTP_CONNECTION_POOL_H
#define _FTP_CONNECTION_POOL_H

#include <www2/SocketConnectionPool.h>

using namespace B::WWW2;

class FtpConnection;

class FtpConnectionPool : public SocketConnectionPool
{
public:

	status_t GetFtpConnection(const char *hostname, int port,
		const char *username, const char *password,
		bool tryToReuse, int *outcode, FtpConnection **connection);

	virtual status_t CreateConnection(const char *hostname, int port,
		Connection **new_connection);

private:

	status_t AttemptLogin(Connection *con, const char *username,
		const char *password, int *outcode, FtpConnection **ftp_connection);
};

#endif // _FTP_CONNECTION_POOL_H
