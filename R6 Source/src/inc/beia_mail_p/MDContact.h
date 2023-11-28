#ifndef __MD_CONACT_H__
#define __MD_CONTACT_H__

#include <List.h>
#include <Entry.h>
#include <String.h>
#include <Locker.h>
#include <File.h>

#define	MD_CONTACT_FIELD_ID	"mc_id"

class BMessage;

class MDContactEntry;

_EXPORT class MDContact : public BList
{
public:
							MDContact();
							MDContact(MDContact &);
virtual						~MDContact();

virtual void 				PrintToStream();
virtual void				Empty();

virtual void 				SetTo(MDContact &);
virtual void				SetTo(BMessage *);
virtual	void				Get(BMessage *);
virtual MDContactEntry *	AddEntry(const char *,const char *);
virtual	void				RemoveEntry(const char *);
virtual MDContactEntry *	FindEntry(const char *);

int64						id;

private:
};

#endif
