#include <stdio.h>
#include "FtpConnectionPool.h"
#include "FtpConnection.h"

status_t FtpConnectionPool::CreateConnection(const char *hostname, int port,
	Connection **new_connection)
{
	FtpConnection *con = new FtpConnection;
	
	status_t error = con->Open(hostname, port);
	if (error >= B_OK) {
		*new_connection = con;
	} else {
		FTP_TRACE(("Connection %08X would not open %s:%d\n",
			reinterpret_cast<unsigned int>(con), hostname, port));
		delete con;
	}
	
	return error;
}

status_t FtpConnectionPool::GetFtpConnection(const char *hostname, int port,
	const char *username, const char *password, bool tryToReuse,
	int *outcode, FtpConnection **ftp_connection)
{
	Connection *con;
	
	// Get some kind of Connection:
	status_t error = GetConnection(hostname, port, tryToReuse, &con);
	if (error >= B_OK) {
		error = AttemptLogin(con, username, password, outcode, ftp_connection);
		if (error <= B_ERROR) {
			// Dump this Connection.
			ReturnConnection(con, 0, 0);
			if (tryToReuse) {
				// Maybe we got a reused Connection, but the FTP server doesn't
				// support REINitializing.  Get a whole new one:
				error = GetConnection(hostname, port, false, &con);
				if (error >= B_OK) {
					error = AttemptLogin(con, username, password, outcode, ftp_connection);
					if (error <= B_ERROR) {
						ReturnConnection(con, 0, 0);
					}
				}
			}
		}
	}

	return error;
}

status_t FtpConnectionPool::AttemptLogin(Connection *con, const char *username,
	const char *password, int *outcode, FtpConnection **ftp_connection)
{
	status_t error;
	// See if it's an FtpConnection:
	FtpConnection *ftp_con = dynamic_cast<FtpConnection *>(con);
	if (ftp_con) {
		if (ftp_con->IsLoggedInAs(username, password)) {
			// The FtpConnection is already logged in under this username and
			// password, so we're done.
			*ftp_connection = ftp_con;
			error = B_OK;
		} else {
			error = ftp_con->Login(username, password, outcode);
			if (error >= B_OK) {
				*ftp_connection = ftp_con;
			} else {
				error = B_ERROR;
			}
		}
	} else {
		FTP_TRACE(("Connection %08X is not an FtpConnection.\n", reinterpret_cast<unsigned int>(con)));
		error = B_ERROR;
	}
	
	return error;
}

