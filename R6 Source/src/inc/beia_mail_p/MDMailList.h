#ifndef __MD_MAILLIST_H__
#define __MD_MAILLIST_H__

#include <List.h>
#include <String.h>

#define	MAIL_LIST_TOC_FILENAME		"mailbox.toc"

#define	MAIL_LIST_TOC_HEADER_MESSAGE		'MAIL'
#define	MAIL_LIST_TOC_HEADER_NB				"nb"

class MDMail;
class MDMailList : public BList
{
public:
					MDMailList();
virtual				~MDMailList();

virtual void 		PrintToStream();
virtual void		Empty();

virtual	void		SetRoot(const char *path);
virtual	const char *Root();

virtual	status_t	LoadList();
virtual	status_t	SaveList();

virtual	status_t	AddMail(MDMail *);
virtual	status_t	RemoveMail(MDMail *);
virtual	status_t	RemoveMail(const char *);

virtual MDMail *	FindMail(const char *);
virtual MDMail *	FindOldestUsedMail(bool with_container = false);
virtual MDMail *	FindOldestMail(bool with_container = false);

virtual	void		SetDirty();

virtual void 		GetMailboxStats(uint32 *total_messages, uint32 *new_messages);

virtual	int64 		DiskSpace();
virtual int64		MemorySpace();

virtual	bool		SyncNeeded();

private:
BString				root;
bool				dirty;
};

#endif
