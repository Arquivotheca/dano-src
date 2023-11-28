#ifndef _VHTUTILS_H_
#define _VHTUTILS_H_

#include <SupportDefs.h>

class HTConnection;
class BMessage;

status_t	GetHttpResponse(HTConnection &ht, 
							char *buffer, int bufsz);

status_t	GetValetResponse(HTConnection &ht, BMessage *msg);

bool		HTMLTextify(char *dst, char *src, bool intag);

extern const char *kRegConnect;
extern const char *kUpdateConnect;
extern const char *kGetUpdateConnect;

#endif
