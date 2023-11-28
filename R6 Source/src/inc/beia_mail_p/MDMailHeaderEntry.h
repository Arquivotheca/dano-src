#ifndef __MD_MAIL_HEADER_ENTRY_H__
#define __MD_MAIL_HEADER_ENTRY_H__

#include <String.h>

#define	MD_MAIL_HEADER_FIELD_NAME	"md_name"
#define	MD_MAIL_HEADER_FIELD_VALUE	"md_value"

class BMessage;

_EXPORT class MDMailHeaderEntry
{
public:
				MDMailHeaderEntry();
				MDMailHeaderEntry(const MDMailHeaderEntry &);
virtual			~MDMailHeaderEntry();

virtual	void 	PrintToStream();

virtual int64 	MemorySpace();

virtual	void	SetTo(const MDMailHeaderEntry &);
virtual	bool	SetTo(BMessage *,int32);
virtual	void	Get(BMessage *);

BString			name;		// name of the field
BString			value;		// value of the field
};

#endif
