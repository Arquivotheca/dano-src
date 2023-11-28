#ifndef __MAIL_CLIENT_RECIPIENTS_LIST_H__
#define __MAIL_CLIENT_RECIPIENTS_LIST_H__

#include <List.h>
#include <Message.h>

_EXPORT class MDMailRecipientsList : public BList
{
public:
				MDMailRecipientsList();
				MDMailRecipientsList(const MDMailRecipientsList &);
virtual			~MDMailRecipientsList();

virtual void 	PrintToStream();
virtual void	Empty();

virtual	void	SetTo(const MDMailRecipientsList &);
virtual	void	SetTo(BMessage *,const char *);
virtual	void	Get(BMessage *,const char *);

virtual	void	Concat(BString *);

virtual int64 	MemorySpace();
};

#endif
