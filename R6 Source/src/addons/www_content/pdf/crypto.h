#if !defined(_CRYPTO_H_)
#define _CRYPTO_H_

#include "rc4.h"
#include "md5.h"
#include <SupportDefs.h>

class RC4codec {
public:
			RC4codec(const uchar *key, int key_length);
			~RC4codec();
			
void		Reset(void);
void		Encode(uchar *buffer, int buffer_length);

private:
int			m_key_length;
uchar 		*m_raw_key;
RC4_KEY		m_rc4_key;
};

class MD5Hash {
public:
			MD5Hash(void);
			~MD5Hash();

void		Reset(void);
void		Update(const uchar *data, uint32 length);
void		Final(void);
const uchar *Digest(void);
private:
MD5_CTX		m_context;
uchar		digest[MD5_DIGEST_LENGTH];
};

#endif
