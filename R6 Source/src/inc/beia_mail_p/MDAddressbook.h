#ifndef __MD_ADDRESSBOOK_H__
#define __MD_ADDRESSBOOK_H__

#include <String.h>

_EXPORT class MDAddressbook
{
public:
				MDAddressbook(const char *);
				MDAddressbook(const MDAddressbook &);
virtual			~MDAddressbook();

virtual void 	PrintToStream();

BString			name;
};

#endif
