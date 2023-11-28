#ifndef __MD_MAILBOXES_LIST_H__
#define __MD_MAILBOXES_LIST_H__

#include <List.h>
#include <String.h>

class MDMailbox;
class MDMailboxesList;
_EXPORT class MDMailboxesList : public BList
{
public:
				MDMailboxesList();
				MDMailboxesList(const MDMailboxesList &);
virtual			~MDMailboxesList();

virtual void 	PrintToStream();
virtual void	Empty();

virtual	void	SetTo(const MDMailboxesList &);

virtual MDMailbox *	AddFolderFromIMAP(const char *);
virtual MDMailbox *	AddFolder(const char *);

virtual	void	RemoveFolder(const char *, const char *);

virtual MDMailbox *	FindFolder(const char *, const char *);
virtual MDMailbox *	FindFolder(const char *);

BString	separator;
};

#endif
