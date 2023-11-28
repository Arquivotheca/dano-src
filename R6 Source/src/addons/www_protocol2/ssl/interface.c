/*
 *	Browser interface
 */

#include <OS.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "openssl/ssl.h"

struct connection_info {
	SSL *m_con;
	BIO *m_sbio;
	X509 *m_cert;
};

static int32 init_count = 0;
static volatile int32 library_initialized = 0;
static SSL_CTX *ssl_context = NULL;
static sem_id locks[CRYPTO_NUM_LOCKS];

/* public hook functions */
int open_secure_connection(void **out_cookie, int sock);
int close_secure_connection(void *cookie);
int read_secure_connection(void *cookie, void *buf, unsigned size);
int write_secure_connection(void *cookie, const void *buf, unsigned size);

/* private functions */
static int ssl_init(void);

int open_secure_connection(void **out_cookie, int sock)
{
	struct connection_info *connection = NULL;
	int err;
	
	if (init_count == 0 && atomic_add(&init_count, 1) == 0) {
		err = ssl_init();
		library_initialized = 1;
		if (err < 0)
			return err;
	}

	/* Wait for another thread to initialize the library */
	while (!library_initialized)
		snooze(100000);

	connection = (struct connection_info*) malloc(sizeof(struct connection_info));
	if (connection == NULL)
		goto error1;
	
	connection->m_con = (SSL*) SSL_new(ssl_context);
	if(connection->m_con == NULL) {
		printf("SSL_new failed\n");
		goto error2;
	}


	connection->m_sbio = BIO_new_socket(sock, BIO_NOCLOSE);
	if(connection->m_sbio == 0) {
		printf("BIO_new_socket failed\n");
		goto error3;
	}
	
	SSL_set_bio(connection->m_con, connection->m_sbio, connection->m_sbio);
	SSL_set_connect_state(connection->m_con);
	
	err = SSL_connect(connection->m_con);
	if (err < 0) {
		printf("SSL_connect failed: error %d\n", SSL_get_error(connection->m_con, err));
		goto error3;
	}

	connection->m_cert = SSL_get_peer_certificate(connection->m_con);
	*out_cookie = (void*) connection;
	return B_OK;

error3:
	SSL_shutdown(connection->m_con);
	SSL_free(connection->m_con);
error2:
	free(connection);
error1:
	return B_ERROR;
}

int close_secure_connection(void *cookie)
{
	struct connection_info *connection = (struct connection_info*) cookie;
	SSL_shutdown(connection->m_con);
	SSL_free(connection->m_con);
	if (connection->m_cert)
		X509_free(connection->m_cert);

	free(connection);
	return B_OK;
}

int read_secure_connection(void *cookie, void *buf, unsigned size)
{
	return SSL_read(((struct connection_info*) cookie)->m_con, buf, size);
}

int write_secure_connection(void *cookie, const void *buf, unsigned size)
{
	return SSL_write(((struct connection_info*) cookie)->m_con, (char*) buf, size);
}

static void ssl_locking_cb(int mode, int lockid, const char *file, int line)
{
	/* Silence the compiler */
	(void)file;
	(void)line;
	
	if (mode & CRYPTO_LOCK) {
		while (acquire_sem(locks[lockid]) == B_INTERRUPTED)
			;
	} else
		release_sem(locks[lockid]);
}

static int ssl_add_lock_cb(int *num, int amount, int lockid, const char *file, int line)
{
	/* Silence the compiler */
	(void)lockid;
	(void)file;
	(void)line;
	
	return atomic_add((int32*) num, amount) + amount;
}

static unsigned long ssl_id_cb()
{
	return find_thread(NULL);
}

static int ssl_init(void)
{
	int i;

	/* Initialize library */
	SSL_library_init();

	/* Set up for multi threading */
	for (i = 0; i < CRYPTO_NUM_LOCKS; i++)
		locks[i] = create_sem(1, "Crypto Lock");

	CRYPTO_set_locking_callback(ssl_locking_cb);
	CRYPTO_set_add_lock_callback(ssl_add_lock_cb);
	CRYPTO_set_id_callback(ssl_id_cb);

	/* Initialize the global context */
	ssl_context = SSL_CTX_new(SSLv23_client_method());
	if (ssl_context == NULL)
		return -1;
	
	SSL_CTX_set_options(ssl_context, SSL_OP_ALL);
	SSL_CTX_set_default_verify_paths(ssl_context);

#if BIG_BROTHER_IS_WATCHING
	#warning Strong encryption is *not* enabled
	SSL_CTX_set_cipher_list(ssl_context, "EXP");
#else
	#warning Strong encryption enabled
	SSL_CTX_set_cipher_list(ssl_context, "DEFAULT");
#endif

	return 0;
}
