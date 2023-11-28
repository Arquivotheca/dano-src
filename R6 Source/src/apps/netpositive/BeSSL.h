
//
// BeSSL.h
// Copyright 1998 by Be Incorporated.
// Class for accessing the Secure Sockets Layer library
//


#ifndef H_BESSL
#define H_BESSL

//
// IMPORTANT!
// This file must be the last one included.
// I will fix this someday. -Howard
//

#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

// now using SSLC instead of SSLeay
#include "openssl/ssl.h"

typedef struct be_ssl_info {
	char ssl_cipher[255];
	char ssl_version[255];
	int  ssl_keylength;
} be_ssl_info_t;

class BeSSL
{
public:

	BeSSL();
	virtual ~BeSSL();
	

	int		Write(const uint8 *buffer, uint32 length) const;
	int		Read(uint8 *buffer, uint32 length) const;
	int		Connect(const char *hostName, short port = 443);
	int		GetSocket() const;
	int		InitCheck() const;
	void	GetInfo(be_ssl_info_t *sit); 
			
private:
    SSL_CTX		*m_ctx;
	SSL 		*m_con;
	BIO 		*m_sbio;

	int          m_init;
	int			 m_socket;
};


#endif // H_BESSL
