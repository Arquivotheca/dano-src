// DestinationList.cpp
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include "DestinationList.h"


DestItem::DestItem(const char *_p, int32 code)
	:	ListItem(),
		path(NULL),
		findCode(code),
		dir(NULL)
{
	path = strdup(_p);
}

DestItem::~DestItem()
{
	free(path);
}

FindItem::FindItem(const char *_name,
				   off_t _size,
				   const char *_app_sig)
	: DestItem(_name),
	  size(_size),
	  queryFailed(FALSE)
{
	fPredList = new RList<char *>;

	char buf[64];
	
	sprintf(buf,"size = %Ld",size);
	AddPredicate(buf);
	if (*_app_sig) {
		sprintf(buf,"BEOS:APP_SIG = %s",_app_sig);
		AddPredicate(buf);
	}
}

FindItem::FindItem(const char *_name)
	:	DestItem(_name)
{
	fPredList = new RList<char *>;
}

FindItem::~FindItem()
{
	for (int32 i = fPredList->CountItems()-1; i >= 0; i--)
		free(fPredList->ItemAt(i));
		
	delete fPredList;
}

void	FindItem::AddPredicate(const char *pred)
{
	fPredList->AddItem(strdup(pred));
}

const char *FindItem::PredicateAt(int32 index) const
{
	return (const char *)fPredList->ItemAt(index);
}

int32	FindItem::CountPredicates() const
{
	return fPredList->CountItems();
}
