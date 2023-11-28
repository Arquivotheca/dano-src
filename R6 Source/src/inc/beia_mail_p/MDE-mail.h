#ifndef _MD_EMAIL_H
#define _MD_EMAIL_H

#include <List.h>
#include <UTF8.h>

/* -----------------------------------------------------------------------*/
/* 'E-Mail' attributes...*/

#define MD_MAIL_ATTR_NAME		"MAIL:name"			/* indexed string*/
#define MD_MAIL_ATTR_STATUS		"MAIL:status"		/* indexed string*/
#define MD_MAIL_ATTR_PRIORITY	"MAIL:priority"		/* indexed string*/
#define MD_MAIL_ATTR_TO			"MAIL:to"			/* indexed string*/
#define MD_MAIL_ATTR_CC			"MAIL:cc"			/* indexed string*/
#define MD_MAIL_ATTR_FROM		"MAIL:from"			/* indexed string*/
#define MD_MAIL_ATTR_SUBJECT		"MAIL:subject"		/* indexed string*/
#define MD_MAIL_ATTR_REPLY		"MAIL:reply"		/* indexed string*/
#define MD_MAIL_ATTR_WHEN		"MAIL:when"			/* indexed time*/
#define MD_MAIL_ATTR_FLAGS		"MAIL:flags"		/* indexed int32*/
#define MD_MAIL_ATTR_RECIPIENTS	"MAIL:recipients"	/* string*/
#define MD_MAIL_ATTR_MIME		"MAIL:mime"			/* string*/
#define MD_MAIL_ATTR_HEADER		"MAIL:header_length"	/* int32*/
#define MD_MAIL_ATTR_CONTENT		"MAIL:content_length"	/* int32*/


/* mail flags */
enum	mail_flags			{MD_MAIL_PENDING	= 1,	/* waiting to be sent*/
							 MD_MAIL_SENT	= 2,	/* has been sent*/
							 MD_MAIL_SAVE	= 4};	/* save mail after sending*/

#define MD_MAIL_TYPE			"text/x-email"			/* mime type*/


/* -----------------------------------------------------------------------*/
/* defines...*/

/* schedule days */
#define MD_CHECK_NEVER			 0
#define MD_CHECK_WEEKDAYS		 1
#define MD_CHECK_DAILY			 2
#define MD_CHECK_CONTINUOUSLY	 3
#define MD_CHECK_CONTINUOSLY		 3

/* max. lengths */
#define MD_MAX_USER_NAME_LENGTH	32
#define MD_MAX_HOST_NAME_LENGTH	64

/* rfc822 header field types */
#define MD_MAIL_TO			"To: "
#define MD_MAIL_CC			"Cc: "
#define MD_MAIL_BCC			"Bcc: "
#define MD_MAIL_FROM		"From: "
#define MD_MAIL_DATE		"Date: "
#define MD_MAIL_REPLY		"Reply-To: "
#define MD_MAIL_SUBJECT		"Subject: "
#define MD_MAIL_PRIORITY	"Priority: "


/* -----------------------------------------------------------------------*/
/* global functions...*/

status_t		get_smtp_host(char*);

ssize_t		decode_base64(char *out, char *in, off_t length,
								bool replace_cr = false);
ssize_t		encode_base64(char *out, char *in, off_t length);

char *utf8_to_rfc2047(uint32 encoding, const char *string, int32 *stringLen);
char *rfc2047_to_utf8(const char *string);


/* -----------------------------------------------------------------------*/
/* class...*/

class BMallocIO;

class MDMailMessage {

public:
					MDMailMessage(void);
					~MDMailMessage(void);

		status_t	AddContent(const char *text, int32 length,
							   uint32 encoding = B_ISO1_CONVERSION,
							   bool clobber = false);
		status_t	AddContent(const char *text, int32 length,
							   const char *encoding, bool clobber = false);

		status_t	AddEnclosure(entry_ref *ref, bool clobber = false);
		status_t	AddEnclosure(const char *path, bool clobber = false);
		status_t	AddEnclosure(const char *MIME_type, void *data, int32 len,
								 bool clobber = false);

		status_t	AddHeaderField(uint32 encoding, const char *field_name, const char *str, 
								   bool clobber = false);
		status_t	AddHeaderField(const char *field_name, const char *str,
								   bool clobber = false);

		status_t	Send(bool send_now = false, bool remove_when_I_have_completed_sending_this_message_to_your_preferred_SMTP_server = false);
		status_t	SendMemory(BMallocIO *);
		
		char *		BuildRecipientsString();
		char *		BuildFromRecipientsString();


/* -----------------------------------------------------------------------*/

private:
		BList*		fFields;
		bool		fMultiPart;

		int32		concatinate(char**, int32, char*);
		int32		count_fields(char *name = NULL);
		status_t	find_field(char*, type_code*, char**, void**, int32*, 
							   uint32*, char**, bool*, int32);
		BList*		find_field(const char*);
		status_t	get_field_name(char**, int32);
		status_t	set_field(const char*, type_code, const char*, const void*,
							  int32, uint32, const char*, bool);
};

#endif /* _MAIL_H */
