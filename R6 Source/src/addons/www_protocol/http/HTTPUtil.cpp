#include "HTTPUtil.h"

inline void EncodeBase64(char *out, const char *in)
{
	const char *kBase64Char = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
	int len = strlen(in);
	while (len > 2) {
		*out++ = kBase64Char[in[0] >> 2];
		*out++ = kBase64Char[((in[0] & 3) << 4) | (in[1] >> 4)];
		*out++ = kBase64Char[((in[1] & 0xf) << 2) | ((in[2] >> 6) & 3)];
		*out++ = kBase64Char[in[2] & 63];
		len -= 3;
		in += 3;
	}
	
	switch (len) {
		case 1:
			*out++ = kBase64Char[in[0] >> 2];
			*out++ = kBase64Char[(in[0] & 3) << 4];
			*out++ = '=';
			*out++ = '=';
			break;
	
		case 2:
			*out++ = kBase64Char[in[0] >> 2];
			*out++ = kBase64Char[((in[0] & 3) << 4) | (in[1] >> 4)];
			*out++ = kBase64Char[(in[1] & 0xf) << 2];
			*out++ = '=';
			break;
	}
	
	*out = '\0';
}

void EncodeBasicAuthentication(StringBuffer &out, const char *user, const char *password)
{
	char encoded[520];
	StringBuffer credentials(256);
	credentials << user << ':' << password;
	EncodeBase64(encoded, credentials.String());
	out << encoded;
}

