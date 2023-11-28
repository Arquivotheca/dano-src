#include <Be.h>
// DestinationList.cpp

#include "DestinationList.h"


DestItem::DestItem( const char *p,
					const char *name,
					int32 code)
	: ListItem(),
	  path(NULL),
	  findName(NULL),
	  findCode(code)
{
	path = strdup(p);
	findName = strdup(name);
}

DestItem::~DestItem()
{
	free(path);
	free(findName);
}

FindItem::FindItem(const char *_name,
					off_t _size,
					const char *_app_sig)
	: DestItem(_name),
	  size(_size),
	  appSig(NULL)
{
	appSig = strdup(_app_sig);
}

FindItem::~FindItem()
{
	if (appSig) free(appSig);
}

void FindItem::SetSignature(const char *s)
{
	if (appSig) free(appSig);
	appSig = strdup(s);
}

const char *FindItem::Signature() const
{
	return appSig;
}
