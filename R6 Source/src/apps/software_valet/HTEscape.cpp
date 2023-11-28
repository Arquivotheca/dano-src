#include "HTEscape.h"
#include <malloc.h>


int HTEscapedLen(const char *v)
{
        int len = 0;
        const unsigned char *s = (const unsigned char *)v;
        unsigned char c;
        while (c = *s++) {
                if ((c < 32) || (c > 127) || strchr(" +&=%/~",c)) {
                        len += 2;
                }
                len++;
        }
        return len;
}

char *HTEscapedString(const char *v)
{
        unsigned char *dst = (unsigned char *)malloc(HTEscapedLen(v)+1);
        if (!dst)
                return (char *)dst;

        const unsigned char *s = (const unsigned char *)v;
        char *res = (char *)dst;
        unsigned char c;
        while (c = *s++) {
                if ((c < 32) || (c > 127) || strchr(" +&=%/~",c)) {
                        int v = c;
                        *dst++ = '%';
                        sprintf((char *)dst,"%02X",v);
                        dst += 2;
                }
                else {
                        *dst++ = c;
                }
        }
        *dst = 0;
        return res;
}

// inplace
void	HTUnescapeString(char *v)
{
	char *c = v;
	while (*v)	{
		if (*c == '%') {
			// treat next two as hex
			*c++;
			//if (*c)		
		}
	}
}

