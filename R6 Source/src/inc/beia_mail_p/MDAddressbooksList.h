#ifndef __MD_ADDRESSBOOKS_LIST_H__
#define __MD_ADDRESSBOOKS_LIST_H__

#include <List.h>

class MDAddressbook;
class MDAddressbooksList;
_EXPORT class MDAddressbooksList : public BList
{
public:
				MDAddressbooksList();
				MDAddressbooksList(const MDAddressbooksList &);
virtual			~MDAddressbooksList();

virtual void 	PrintToStream();
virtual void	Empty();

virtual	void	SetTo(const MDAddressbooksList &);

virtual MDAddressbook *	AddFolder(const char *);

virtual	void	RemoveFolder(const char *, const char *);

virtual MDAddressbook *	FindFolder(const char *, const char *);
virtual MDAddressbook *	FindFolder(const char *);
};

#endif
