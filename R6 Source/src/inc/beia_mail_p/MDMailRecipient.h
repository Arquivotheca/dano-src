#ifndef __MD_MAIL_RECIPIENT_H__
#define __MD_MAIL_RECIPIENT_H__

#include <String.h>
#include <OS.h>
#include <Message.h>

_EXPORT class MDMailRecipient
{
public:
				MDMailRecipient(const char *name = NULL, const char *email = NULL);
				MDMailRecipient(const MDMailRecipient &);
virtual			~MDMailRecipient();

virtual	void 	PrintToStream();

virtual	void	SetTo(const MDMailRecipient &);
virtual	bool	SetTo(BMessage *,const char *,int32 index = -1);
virtual	void	Get(BMessage *,const char *);

virtual	void	Concat(BString *);

virtual int64 	MemorySpace();

BString		name;
BString		email;
};

#endif
