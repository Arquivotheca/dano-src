#include <kernel/image.h>
#include <support2/Debug.h>
#include <support2/Locker.h>
#include <www2/SSLConnection.h>

#include <signal.h>
#include <unistd.h>

namespace B {
namespace WWW2 {

const char *kSSLLibPath = "/system/add-ons/web/ssl.so";

BLocker SSLConnection::fEncryptionLibraryLock("SSL Module Lock");
int SSLConnection::fEncryptionLibraryRefCount = 0;
image_id SSLConnection::fEncryptionLibraryImage = B_BAD_IMAGE_ID;
int (*SSLConnection::fOpenEncryptedConnection)(void **out_cookie, int sock);
int (*SSLConnection::fCloseEncryptedConnection)(void *cookie);
int (*SSLConnection::fReadEncryptedConnection)(void *cookie, void *buf, unsigned size);
int (*SSLConnection::fWriteEncryptedConnection)(void *cookie, const void *buf, unsigned size);

} } // namespace B::WWW2;

using namespace B::WWW2;

SSLConnection::SSLConnection()
	:	fEncryptionCookie(NULL)
{
}

SSLConnection::~SSLConnection()
{
	Close();
}

status_t SSLConnection::Open(const char *host, int port)
{
	status_t error = OpenEncryptedConnection(host, port);
	if (error < 0) {
		Close();
		return error;
	}
	
	return B_OK;
}


void SSLConnection::Close()
{
	if (fEncryptionCookie) {
		(*fCloseEncryptedConnection)(fEncryptionCookie);
		fEncryptionCookie = NULL;
		PutEncryptionModule();
	}
	
	BufferedConnection::Close();
}

bool SSLConnection::IsConnected() const
{
	//(Open)SSL connections aren't re-usable - don't bother
	return false;
}

status_t SSLConnection::OpenUnencrypted(const char *host, int port)
{
	return BufferedConnection::Open(host, port);
}

status_t SSLConnection::EncryptConnection()
{
	status_t error;
	if (!fEncryptionCookie) {
		error = GetEncryptionModule();
		if (error >= B_OK)
			error = (*fOpenEncryptedConnection)(&fEncryptionCookie, Socket());
	}
	else {
		error = B_OK;
	}
	
	return error;
}

status_t SSLConnection::OpenEncryptedConnection(const char *host, int port)
{
	status_t error = OpenUnencrypted(host, port);
	if (error >= B_OK)
		error = EncryptConnection();

	return error;
}


ssize_t SSLConnection::UnbufferedRead(void *dest_buffer, size_t count)
{
	ssize_t retval;
	if (fEncryptionCookie)
		retval = (*fReadEncryptedConnection)(fEncryptionCookie, dest_buffer, count);
	else
		retval = B_ERROR;
	
	return retval;
}


ssize_t SSLConnection::UnbufferedWrite(const void *source_buffer, size_t count)
{
	ssize_t retval;
	if (fEncryptionCookie)
		retval = (*fWriteEncryptedConnection)(fEncryptionCookie, source_buffer, count);
	else
		retval = B_ERROR;
		
	return retval;
}


status_t SSLConnection::GetEncryptionModule()
{
	fEncryptionLibraryLock.Lock();
	fEncryptionLibraryRefCount++;
	if (fEncryptionLibraryImage == B_BAD_IMAGE_ID) {
		PRINT(("Loading SSL library.\n"));
		if ((fEncryptionLibraryImage = load_add_on(kSSLLibPath)) < 0
			|| get_image_symbol(fEncryptionLibraryImage, "open_secure_connection", B_SYMBOL_TYPE_TEXT,
				reinterpret_cast<void**>(&fOpenEncryptedConnection)) < 0
			|| get_image_symbol(fEncryptionLibraryImage, "close_secure_connection", B_SYMBOL_TYPE_TEXT,
				reinterpret_cast<void**>(&fCloseEncryptedConnection)) < 0
			|| get_image_symbol(fEncryptionLibraryImage, "read_secure_connection", B_SYMBOL_TYPE_TEXT,
				reinterpret_cast<void**>(&fReadEncryptedConnection)) < 0
			|| get_image_symbol(fEncryptionLibraryImage, "write_secure_connection", B_SYMBOL_TYPE_TEXT,
				reinterpret_cast<void**>(&fWriteEncryptedConnection)) < 0) {
			fprintf(stderr, "Error loading SSL library.\n");
			fEncryptionLibraryLock.Unlock();
			return B_ERROR;
		}
	}

	fEncryptionLibraryLock.Unlock();
	return B_OK;
}

void SSLConnection::PutEncryptionModule()
{
	fEncryptionLibraryLock.Lock();
	if (--fEncryptionLibraryRefCount == 0) {
#if 0
		// Currently the library is not unloaded because it gets loaded and unloaded
		// a lot when accessing encrypted sites.  It needs to be able to time out
		// eventually.
		PRINT(("Unloading SSL library\n"));
		unload_add_on(fEncryptionLibraryImage);
		fEncryptionLibraryImage = B_BAD_IMAGE_ID;
#endif
	}

	fEncryptionLibraryLock.Unlock();
}

