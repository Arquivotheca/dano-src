#ifndef __MD_MAIL_HEADER_H__
#define __MD_MAIL_HEADER_H__

#include <String.h>
#include <List.h>
#include <OS.h>
#include <Message.h>

#include "MDMailRecipient.h"
#include "MDMailRecipientsList.h"
#include "MDMailDefs.h"

#define	MD_MAIL_HEADER_FIELD_BCC			"md_bcc"
#define	MD_MAIL_HEADER_FIELD_DATE			"md_date"
#define	MD_MAIL_HEADER_FIELD_SIZE			"md_size"
#define	MD_MAIL_HEADER_FIELD_STATUS			"md_status"
#define	MD_MAIL_HEADER_FIELD_LABEL			"md_label"
#define	MD_MAIL_HEADER_FIELD_SYNC_NEEDED	"md_sync"

#define	MD_MAIL_HEADER_FIELD_FILENAME		"md_filename"
#define	MD_MAIL_HEADER_FIELD_UID			"md_uid"
#define	MD_MAIL_HEADER_FIELD_TAG			"md_tag"
#define	MD_MAIL_HEADER_FIELD_CONTENT_LOADED	"md_cl"

class MDMailHeaderEntry;

_EXPORT class MDMailHeader : public BList
{
public:
						MDMailHeader();
						MDMailHeader(const MDMailHeader &);
virtual					~MDMailHeader();

virtual void 			PrintToStream();
virtual void			Empty();

virtual void			SetTo(const MDMailHeader &);
virtual void			SetTo(BMessage *);
virtual void			Get(BMessage *);

virtual	void			Tag();

virtual MDMailHeaderEntry *	FindEntry(const char *);
virtual MDMailHeaderEntry *	AddEntry(const char *,const char *);
virtual int32				CountEntries(const char *);
virtual MDMailHeaderEntry *	GetEntry(const char *,int32);

virtual	void				GenerateUID();

virtual int64 				MemorySpace();

MDMailRecipientsList	bcc;			// recipients of the message (bcc)

time_t					date;			// date of the message
off_t					size;			// size of the message
md_mail_status			status;			// status of the message
int8					label;			// label of the message

BString					filename;		// filename of the file used to store the data
BString					uid;			// message identifier

uint32					tag;			// tag used to know the last use of this message
bool					sync_needed;	// sync needed when online (status changes for example)
bool					content_loaded;	// flag to know if the content is loaded
};

#endif
