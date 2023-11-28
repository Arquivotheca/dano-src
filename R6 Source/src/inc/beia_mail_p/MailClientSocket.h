#ifndef _MAIL_CLIENT_SOCKET_H
#define _MAIL_CLIENT_SOCKET_H

#include <String.h>
#include <image.h>
#include "Gehnaphore.h"

class MailClientSocket
{
public:
	MailClientSocket();
	~MailClientSocket();
	status_t Connect(const char *hostname, int port, bool secure = false);
	void Close();
	bool IsOpen() const;
	bool IsSecure() const;

int32 		SendString(const char *);
int32 		SendData(const char *,int32);
	
bool 		RecvString(BString *,bool lf=false);
bool 		RecvString_raw(BString *);

private:
	status_t OpenRawSocket(const char *hostname, int port);
	ssize_t UnbufferedRead(void *buffer, int size);
	ssize_t UnbufferedWrite(const void *buffer, int size);
	static status_t GetSSLModule();
	static void PutSSLModule();

	Gehnaphore fLock;

	int fSocketFD;
	void *fSSLCookie;

char		RecvCar(void);

int32 		nbreceived;
int32		indexreceived;
char		datareceived[4096];
bool		erreur_reception;

	// The SSL library is loaded as a separate add-on.  All of the library hooks
	// are thread safe.  The lock protects multiple threads from trying to load
	// or unload the library at the same time.
	static BLocker fSSLLock;
	static int32 fSSLRefCount;
	static image_id fSSLImage;
	static int (*fOpenSecureConnection)(void **out_cookie, int sock);
	static int (*fCloseSecureConnection)(void *cookie);
	static int (*fReadSecureConnection)(void *cookie, void *buf, unsigned size);
	static int (*fWriteSecureConnection)(void *cookie, const void *buf, unsigned size);
};

#endif
