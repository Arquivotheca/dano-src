//
// BeSSL.cpp
// Copyright 1998 by Be Incorporated.
// Class for accessing the Secure Sockets Layer library
//

#include <OS.h>
#include <errno.h>
#include <ctype.h>
#include "BeSSL.h"
#include <openssl/rand.h>
#include <unistd.h>
#include <arpa/inet.h>
#include "Protocols.h"

#define closesocket close


//
// Sorry for this macro
//
#define SSL_THROW(x, y) if((m_init = (x)) != 0) \
	{ fprintf(stderr, "SSL Initialization error: %s\n",(y)); return; }


BeSSL::BeSSL()
{
	m_ctx = 0;
	m_socket = -1;
	m_init = 0;
	m_con = 0;
	m_sbio = 0;

	static int32 library_init_count = 0;
	static bool library_initialized = false;
	if (atomic_or(&library_init_count, 1) == 0)
	{
		SSL_library_init();
		SSL_load_error_strings();
		library_initialized = true;
	}
	else
	{
		while (!library_initialized) snooze(100000);
	}
	
	m_ctx=SSL_CTX_new(SSLv23_client_method());
	if(m_ctx == 0)
		SSL_THROW(-1, "SSL_CTX_new");

	SSL_CTX_set_options(m_ctx, SSL_OP_ALL);
	SSL_CTX_set_default_verify_paths(m_ctx);

	m_con=(SSL*) SSL_new(m_ctx);
	if(m_con == 0)
		SSL_THROW(-1, "SSL_new");

// Borrowed from www_protocols library
#ifdef EXPORT
	#warning Weak
	SSL_set_cipher_list(m_con, SSL2_TXT_RC4_128_EXPORT40_WITH_MD5); 
#else
	#warning Strong
	//	both strong and weak RC4 cyphers
//	SSL_set_cipher_list(m_con, SSL2_TXT_RC4_128_WITH_MD5":"SSL2_TXT_RC4_128_EXPORT40_WITH_MD5);
	SSL_set_cipher_list(m_con, SSL_TXT_ALL);
#endif
}



BeSSL::~BeSSL()
{
	SSL_set_quiet_shutdown(m_con, 1);
	SSL_shutdown(m_con);
	if (m_con != NULL) SSL_free(m_con);
	if (m_ctx != NULL) SSL_CTX_free(m_ctx);
			
	if(m_socket != -1) closesocket(m_socket);
	m_socket = -1;
}

	
int BeSSL::Write(const uint8 *buffer, uint32 length) const
{
	int rc = SSL_write(m_con, (char *) buffer, length);
	return rc;
}


int BeSSL::Read(uint8 *buffer, uint32 length) const
{
	int rc = SSL_read(m_con, (char *) buffer, length);
	return rc;
}


int  BeSSL::Connect(const char *hostaddr, short port)
{
    struct sockaddr_in sa;
	ulong addr;
	
	//do thread-safe hostname lookup (note that 'addr' is in host-byte
	//order, for some reason)
	addr = DNSLooper::GetHostByName(hostaddr);
	if (addr == 0 || addr == INADDR_NONE)
		return -1;

	memset(&sa, 0, sizeof(sa));
	sa.sin_family = AF_INET;
	sa.sin_len = sizeof(sa);
	sa.sin_port = htons(port);
	sa.sin_addr.s_addr = htonl(addr);

    if ((m_socket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    	m_socket = -1;
    	return -1;
    }
	
    if (connect(m_socket, (struct sockaddr *) &sa, sizeof(sa)) != 0)
    {
		closesocket(m_socket);
		m_socket = -1;
		return -1;
    }

	m_sbio = BIO_new_socket(m_socket, BIO_NOCLOSE);
	if(m_sbio == 0)
	{
		closesocket(m_socket);
		m_socket = -1;
		return -1;	
	}
	
	SSL_set_bio(m_con,m_sbio,m_sbio);
	SSL_set_connect_state(m_con);
	
	int ret = SSL_connect(m_con);
	if (ret != 1) {
		//we failed to connect - find out why
		return SSL_get_error(m_con, ret);
	}
	
	return 0;
}

int BeSSL::GetSocket() const
{
	return m_socket;
}

int BeSSL::InitCheck() const
{
	return m_init;
}

void BeSSL::GetInfo(be_ssl_info_t *sit)
{
	const char *s;
	SSL_CIPHER *c;

	if(m_con == 0 || sit == 0)
	{
		return;
	}
	
	c = SSL_get_current_cipher(m_con);
	
	if(c == 0)
	{
		return;
	}
		
	memset(sit, 0, sizeof(be_ssl_info_t));
	
	s = (char *) SSL_get_version(m_con);
	if(s != 0)
		strcpy(sit->ssl_version, s);
	
	s = (char *) SSL_CIPHER_get_name(c);
	
	if(s != 0)
		strcpy(sit->ssl_cipher, s);
	
	SSL_CIPHER_get_bits(c, &(sit->ssl_keylength));
	
	/*
	 * set to the secret key size for the export ciphers...
	 * SSLeay returns the total key size
	 */
	if(strncmp(sit->ssl_cipher, "EXP-", 4) == 0)
		sit->ssl_keylength = 40;
}



#if DEBUG == 10
static void dumpData(char *data, size_t len, char *func)
{
	int i = 0;
	printf("----------\ndumping %d bytes from %s:\n", len, func);
	char linebuf[17];
	memset(linebuf, 0, sizeof(linebuf));
	while(i<len)
	{
		printf("%02x ", (unsigned char )data[i]);
		if(isprint(data[i]))
			linebuf[i % 16] = data[i];
		else
			linebuf[i % 16] = '.';
			
		if((i + 1) % 16 == 0) {
			printf("  |  %s\n", linebuf);
			memset(linebuf, 0, sizeof(linebuf));
			}
		i++;
	}
	if((i + 1)%16 != 0)
	{
		while(i % 16 != 0)
		{
			printf("   ");
			i++;
		}
		printf("  |  %s", linebuf);
	}
	printf("\n----------\n");
	fflush(stdout);
}
#endif

