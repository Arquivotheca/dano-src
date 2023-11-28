#include "crypto.h"
#include <string.h>


RC4codec::RC4codec(const uchar *key, int key_length)
	: m_key_length(key_length), m_raw_key(new uchar[key_length])
{
	// copy key bytes
	memcpy(m_raw_key, key, m_key_length);
	// prep the key
	RC4_set_key(&m_rc4_key, m_key_length, m_raw_key);
}


RC4codec::~RC4codec()
{
	delete [] m_raw_key;
}

void 
RC4codec::Reset(void)
{
	// prep the key
	RC4_set_key(&m_rc4_key, m_key_length, m_raw_key);
}

void 
RC4codec::Encode(uchar *buffer, int buffer_length)
{
	RC4(&m_rc4_key, buffer_length, buffer, buffer);
}


MD5Hash::MD5Hash(void)
{
	MD5_Init(&m_context);
}


MD5Hash::~MD5Hash()
{
}

void 
MD5Hash::Reset(void)
{
	MD5_Init(&m_context);
}

void 
MD5Hash::Update(const uchar *data, uint32 length)
{
	MD5_Update(&m_context, (uchar *)data, length);
}

void 
MD5Hash::Final(void)
{
	MD5_Final(digest, &m_context);
}

const uchar *
MD5Hash::Digest(void)
{
	return digest;
}

