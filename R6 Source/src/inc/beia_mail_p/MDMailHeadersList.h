#ifndef __MD_HEADERS_LIST_H__
#define __MD_HEADERS_LIST_H__

#include <List.h>

_EXPORT class MDMailHeadersList : public BList
{
public:
				MDMailHeadersList();
				MDMailHeadersList(const MDMailHeadersList &);
virtual			~MDMailHeadersList();

virtual void 	PrintToStream();

virtual void	Empty();
};

#endif
