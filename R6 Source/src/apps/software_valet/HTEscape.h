#ifndef _HTESCAPE_H_
#define _HTESCAPE_H_


#include <string.h>
#include <stdio.h>

int HTEscapedLen(const char *v);
char *HTEscapedString(const char *v);
void	HTUnescapeString(char *v);

inline bool IsURLEscaped(char c)
{
	return (strchr(" +&=%/~",c));
}

inline void URLEscapeChar(char *dst, char c)
{
	sprintf(dst,"%%%02X",(unsigned int)c);
}


#endif
