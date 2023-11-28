#ifndef __MD_CONTACTLIST_H__
#define __MD_CONTACTLIST_H__

#include <List.h>
#include <String.h>

#define	CONTACT_LIST_TOC_FILENAME		"contact.toc"

#define	CONTACT_LIST_TOC_HEADER_MESSAGE			'CONT'
#define	CONTACT_LIST_TOC_HEADER_NB				"nb"

class MDContact;
class MDContactList : public BList
{
public:
					MDContactList();
virtual				~MDContactList();

virtual void 		PrintToStream();
virtual void		Empty();

virtual	void		SetRoot(const char *path);
virtual	const char *Root();

virtual	status_t	LoadList();
virtual	status_t	SaveList();

virtual	status_t	AddContact(MDContact *);
virtual	status_t	RemoveContact(MDContact *);
virtual	status_t	RemoveContact(int64);

virtual MDContact *	FindContact(int64);

virtual	void		SetDirty();

private:
BString				root;
bool				dirty;
};

#endif
