#ifndef _SSL_CONNECTION_H
#define _SSL_CONNECTION_H

#include <kernel/image.h>
#include "BufferedConnection.h"

class SSLConnection : public BufferedConnection {
public:

	SSLConnection();
	~SSLConnection();
	
	// Inherited:
	virtual ssize_t UnbufferedRead(void *dest_buffer, size_t length);
	virtual ssize_t UnbufferedWrite(const void *source_buffer, size_t length);
	virtual status_t Open(const char *host, int port);
	virtual void Close();
	virtual bool HasUnreadData();
	
	// Additional interface:
	status_t OpenUnencrypted(const char *host, int port);
	status_t EncryptConnection();
	
private:

	status_t OpenEncryptedConnection(const char *host, int port);
	static status_t GetEncryptionModule();
	static void PutEncryptionModule();

	void *fEncryptionCookie;

	// The SSL library is loaded as a separate add-on.  All of the library hooks
	// are thread safe.  The lock protects multiple threads from trying to load
	// or unload the library at the same time.
	static BLocker fEncryptionLibraryLock;
	static int fEncryptionLibraryRefCount;
	static image_id fEncryptionLibraryImage;
	static int (*fOpenEncryptedConnection)(void **out_cookie, int sock);
	static int (*fCloseEncryptedConnection)(void *cookie);
	static int (*fReadEncryptedConnection)(void *cookie, void *buf, unsigned size);
	static int (*fWriteEncryptedConnection)(void *cookie, const void *buf, unsigned size);
};

#endif // _SSL_CONNECTION_H
