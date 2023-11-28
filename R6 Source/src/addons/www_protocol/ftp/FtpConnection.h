#ifndef _FTP_CONNECTION_H
#define _FTP_CONNECTION_H

#include <String.h>
#include <SocketConnection.h>

#if ENABLE_FTP_TRACE
	#define FTP_REQUEST_COLOR "\e[0;31m"
	#define FTP_RESPONSE_COLOR "\e[0;34m"
	#define FTP_NORMAL_COLOR "\e[0m"
	#define FTP_STATUS_COLOR "\e[0;35m"

	#define FTP_TRACE(x) printf x
#else
	#define FTP_TRACE(x) ;
#endif

class FtpConnection : public SocketConnection {
public:
	
	FtpConnection()
		:	fLoggedIn(false)
	{ }
	
	// Added interface:
	//  Login() REINitializes if we're already logged in as someone else.
	status_t Login(const char *username, const char *password, int *outcode);
	bool IsLoggedInAs(const char *username, const char *password) const;
	status_t SendRequest(const BString &cmd);
	status_t GetReply(BString *outstr, int *outcode, int *codetype);

private:
	
	status_t GetReplyLine(BString *line);
	status_t Reinitialize();
	
	BString fUsername;
	BString fPassword;
	bool fLoggedIn;
};

#endif // _FTP_CONNECTION_H
