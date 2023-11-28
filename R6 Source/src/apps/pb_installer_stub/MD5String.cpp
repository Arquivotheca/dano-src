//
#include "md5lib.h"
#include <SupportDefs.h>
#include <string.h>

void MD5String(const char *string, uchar* cksum);

void MD5String(const char *string, uchar* cksum)
{
	MD5_CTX	context;
	MD5Init(&context);
	MD5Update(&context,(uchar *)string,strlen(string));
	MD5Final(cksum,&context);
}