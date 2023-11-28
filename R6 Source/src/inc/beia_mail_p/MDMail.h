#ifndef __MD_MAIL_H__
#define __MD_MAIL_H__

#include <List.h>
#include <Entry.h>
#include <String.h>
#include <Locker.h>
#include <File.h>

#include "MDMailDefs.h"
#include "MDMailHeader.h"
#include "MDEncodeBase64.h"

class BMallocIO;
class MDMailContainer;
class MDMailHeader;

_EXPORT class MDMail : public BList
{
public:
					MDMail();
					MDMail(MDMail &);
					MDMail(MDMailHeader &);
virtual				~MDMail();

virtual void 		PrintToStream();
virtual void		Empty();

virtual void		SetTo(MDMail &, bool structureonly = false);
virtual void		SetTo(MDMailHeader &);
virtual void		SetTo(BMessage *);
virtual	void		Get(BMessage *);

virtual	status_t	AddContentStart(MDMailContainer *);
virtual	status_t	AddContent(BMallocIO *,const MDMailContainer *, bool encoded = false);
virtual	status_t	AddContentEnd(MDMailContainer *);

virtual	status_t	GetContentStart(MDMailContainer *,int64 *);
virtual	status_t	GetContent(BMallocIO *,MDMailContainer *,int64, bool *);
virtual status_t	GetContentEnd();

virtual	int32		CountContainers(bool fullonly = false);
virtual	bool		ContainsEnclosures();

virtual	void		SetRoot(const char *path);
virtual	const char *Root();

virtual int64						DiskSpace();
virtual int64						ServerSpace(bool textonly = false);
virtual int64						MemorySpace();
virtual	md_mail_loading_state		LoadingState();

MDMailContainer						*FindSection(const char *);

virtual	status_t					Delete();
virtual	status_t					DeleteContent();

// mail headers
MDMailHeader	header;
BFile			file;

private:
BLocker			file_locker;
BString			root;
int64 			fileindex;			// used to add content
MDEncodeBase64	base64_buffer;		// used to add content

int64			read_index;			// used to get content
};

#endif
