#ifndef __MD_CONTACT_ENTRY_H__
#define __MD_CONTACT_ENTRY_H__

#include <String.h>

#define	MD_CONTACT_FIELD_NAME	"mc_name"
#define	MD_CONTACT_FIELD_VALUE	"mc_value"

_EXPORT class MDContactEntry
{
public:
				MDContactEntry();
				MDContactEntry(const MDContactEntry &);
virtual			~MDContactEntry();

virtual	void 	PrintToStream();

virtual	void	SetTo(const MDContactEntry &);
virtual bool	SetTo(BMessage *,int32);
virtual	void	Get(BMessage *);

BString			name;		// name of the field
BString			value;		// value of the field
};

#endif
