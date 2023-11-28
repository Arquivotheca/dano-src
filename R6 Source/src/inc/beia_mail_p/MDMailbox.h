#ifndef __MD_MAILBOX_H__
#define __MD_MAILBOX_H__

#include <String.h>

_EXPORT class MDMailbox
{
public:
				MDMailbox(const char *);
				MDMailbox(const MDMailbox &);
virtual			~MDMailbox();

virtual void 	PrintToStream();

BString			name;
uint32			total_messages;
uint32			new_messages;
};

#endif
