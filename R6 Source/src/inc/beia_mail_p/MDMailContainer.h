#ifndef __MD_MAILCONTAINER_H__
#define __MD_MAILCONTAINER_H__

#include <String.h>
#include <OS.h>
#include <Message.h>

#define	MD_MAIL_CONTAINER_FIELD_CONTENT_TYPE		"content_type"
#define	MD_MAIL_CONTAINER_FIELD_CONTENT_SUBTYPE		"content_subtype"
#define	MD_MAIL_CONTAINER_FIELD_CHARSET				"charset"
#define	MD_MAIL_CONTAINER_FIELD_SECTION				"section"
#define	MD_MAIL_CONTAINER_FIELD_ENCODING			"encoding"
#define	MD_MAIL_CONTAINER_FIELD_FILENAME			"filename"
#define	MD_MAIL_CONTAINER_FIELD_TYPE				"type"
#define	MD_MAIL_CONTAINER_FIELD_SIZE				"size"
#define	MD_MAIL_CONTAINER_FIELD_LOCALSIZE			"local_size"
#define	MD_MAIL_CONTAINER_FIELD_INDEX				"index"

typedef enum md_mail_encoding
{
	md_mail_encoding_none = 'none',
	md_mail_encoding_7bit = '7bit',
	md_mail_encoding_8bit = '8bit',
	md_mail_encoding_base64 = 'ba64',
	md_mail_encoding_uuencode = 'uuen',
} md_mail_encoding;

typedef enum md_mail_type
{
	md_mail_type_none = 'none',
	md_mail_type_mixed = 'mixe',
	md_mail_type_alternative = 'alte',
} md_mail_type;

_EXPORT class MDMailContainer
{
public:
				MDMailContainer();
				MDMailContainer(MDMailContainer &);
virtual			~MDMailContainer();

virtual void 	PrintToStream();

virtual	void	SetTo(MDMailContainer &);
virtual	bool	SetTo(BMessage *,int32);
virtual	void	Get(BMessage *);

virtual int64	MemorySpace();


virtual void	SetData(const char *data, int32 length);

// image/png
BString					content_type;			// image
BString					content_subtype;		// png

int16					charset;				// -1 = no charset, otherwise see UTF8.h
md_mail_encoding		encoding;
BString					filename;
BString					section;
md_mail_type			type;
int64					size;
int64					local_size;
int64					index;

char					*data;
};

#endif
