#ifndef _HTTP_UTIL_H
#define _HTTP_UTIL_H

#include <StringBuffer.h>

#if ENABLE_HTTP_TRACE
	#define HTTP_REQUEST_COLOR "\e[0;31m"
	#define HTTP_RESPONSE_COLOR "\e[0;34m"
	#define HTTP_NORMAL_COLOR "\e[0m"
	#define HTTP_STATUS_COLOR "\e[0;35m"

	#define HTTP_TRACE(x) printf x
#else
	#define HTTP_TRACE(x) ;
#endif

void EncodeBase64(char *out, const char *in);
void EncodeBasicAuthentication(StringBuffer &out, const char *user, const char *password);

#endif
