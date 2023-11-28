//******************************************************************************
//
//	File:			E-mail.cpp
//
//	Description:	Emailer Session handler.
//
//	Written by:		Robert Polic
//
//	Copyright 1996, Be Incorporated
//
//******************************************************************************

#include <sys/socket.h>
#include <ctype.h>
#include <pwd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>

#include <Alert.h>
#include <Beep.h>
#include <Directory.h>
#include <Entry.h>
#include <File.h>
#include <FindDirectory.h>
#include <fs_attr.h>
#include <E-mail.h>
#include <NodeInfo.h>
#include <parsedate.h>
#include <Path.h>
#include <Roster.h>
#include <Volume.h>
#include <AppFileInfo.h>
#include <mail.h>

#define NC_MAXVALLEN	256

typedef struct
{
	bool enabled;
	char default_domain[256];
} mail_prefs_data;

status_t get_mail_prefs_data(mail_prefs_data *data);

// structures...
typedef struct {
	char		*name;
	BList		*list;
} field;

typedef struct {
	type_code	type;
	void		*data;
	int32		len;
	uint32		encoding;
	char		*encoding_str;
	char		*mime_type;
	bool		needsQP;
} field_item;

enum		FIELD_TYPES		{CONTENT_TEXT, CONTENT_STRING, CONTENT_ENCLOSURE,
							 HEADER_STRING};

// forward declarations...
static int32	file_index = 0;

static void		add_data(BFile*, char*, char*, void*, int32);
static status_t	add_file(BFile*, char*, char*);
static int32	cistrcmp(const char*, const char*);
static void		empty_list(BList*);
void get_encryption(char*);
static status_t	make_file(BEntry*, BFile*, bool = false);
static void		verify_recipients(char**);
static ssize_t	write(BFile*, char*, char*);
static bool		is_8bit_data(const char *string, int32 stringLen);
static char*	qp_encode(const char *string, int32 *stringLen, bool strict, bool *malloced);
static int32	b64_encode(char *dst, const char *src, int32 srcLen);
static char*	encode_header(uint32 encoding, const char *name, const char *data, int32 *dataLen);
static char*	utf8_to_rfc2047(uint32 encoding, const char *string, int32 *stringLen);
static char*	concat_str(char *dst, int32 *dstLen, const char *src, int32 srcLen);
static int32	force_kanjiout(char *dst, int32 dstLen, int32 *convState);

static char *MAIL_VERSION = "BeOS Mail";

inline bool
needs_qp_encode(
	uchar	theChar,
	bool	strict)
{
	return ( (theChar > 0x7F) || (theChar == '=') || 
			 ((strict) && ((theChar == '\t') || (theChar == '_'))) );
}

// private defines...
#define B_MAIL_CONTENT		"Content-Type: "
#define B_MAIL_ENCODING		"Content-Transfer-Encoding: "
#define B_MAIL_XMAILER		"X-Mailer: "
#define B_MAIL_BODY			"Body"

static char basis_64[] =
   "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";

static char index_64[128] = {
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,-1,
    -1,-1,-1,-1, -1,-1,-1,-1, -1,-1,-1,62, -1,-1,-1,63,
    52,53,54,55, 56,57,58,59, 60,61,-1,-1, -1,-1,-1,-1,
    -1, 0, 1, 2,  3, 4, 5, 6,  7, 8, 9,10, 11,12,13,14,
    15,16,17,18, 19,20,21,22, 23,24,25,-1, -1,-1,-1,-1,
    -1,26,27,28, 29,30,31,32, 33,34,35,36, 37,38,39,40,
    41,42,43,44, 45,46,47,48, 49,50,51,-1, -1,-1,-1,-1
};

#define char64(c)  (((c) < 0 || (c) > 127) ? -1 : index_64[(c)])

const uint32 kNoConversion = 32975;
// treat MS CP 1252 as ISO-1, the rest of the world does it, so why shouldn't we?
const char *kCharsets[] = {"ISO-8859-1", "ISO-8859-2", "ISO-8859-3", "ISO-8859-4",
				  		   "ISO-8859-5", "ISO-8859-6", "ISO-8859-7", "ISO-8859-8",
						   "ISO-8859-9", "ISO-8859-10", "x-mac-roman",
						   "x-sjis", "x-euc-jp", "ISO-2022-JP", "ISO-8859-1", "UNICODE", 
						   "KOI8-R", "windows-1251", "dos-866", "dos-437", "euc-kr",
						   "ISO-8859-13", "ISO-8859-14", "ISO-8859-15", "windows-1250",
						   "windows-1253","windows-1254","windows-1255","windows-1256",
						   "windows-1257","windows-1258","dos-737", "dos-775", "dos-850",
						   "dos-852", "dos-855", "dos-857", "dos-860", "dos-861","dos-862",
						   "dos-863","dos-864", "dos-865", "dos-869", "dos-874"};

status_t check_for_mail(int32 *count)
{
	status_t	result = B_MAIL_NO_DAEMON;
	BMessage	msg;
	BMessage	reply;
	BMessenger	*messenger;

	messenger = new BMessenger("application/x-vnd.Be-POST", -1, NULL);
	if (messenger->IsValid()) {
		if (count) {
			msg.what = MAIL_RECEIVE_SYNC;
			result = messenger->SendMessage(&msg, &reply);
			if ((result == B_NO_ERROR) && (reply.what != B_NO_REPLY)) {
				result = reply.FindInt32("result");
				*count = reply.FindInt32("count");
			}
		}
		else {
			msg.what = MAIL_RECEIVE;
			result = messenger->SendMessage(&msg);
		}
	}
	delete messenger;
	return result;
}

//------------------------------------------------------------------

status_t send_queued_mail(void)
{
	status_t	result = B_MAIL_NO_DAEMON;
	BMessage	msg;
	BMessage	*reply;
	BMessenger	*messenger;

	messenger = new BMessenger("application/x-vnd.Be-POST", -1, NULL);
	if (messenger->IsValid()) {
		msg.what = MAIL_SEND;
		result = messenger->SendMessage(&msg);
	}
	delete messenger;
	return result;
}

//------------------------------------------------------------------

status_t forward_mail(entry_ref *ref, const char* recipients, bool now)
{
	void			*buffer;
	char			data[B_FILE_NAME_LENGTH];
	char			new_name[B_FILE_NAME_LENGTH];
	char			*name;
	int32			flags;
	int32			size = 0;
	status_t		result = B_ERROR;
	ssize_t			read;
	BDirectory		dir;
	BEntry			entry;
	BFile			file;
	BFile			*new_file;
	BNodeInfo		*node;
	attr_info		info;

	name = (char *)malloc(strlen(recipients) + 1);
	strcpy(name, recipients);
	verify_recipients(&name);
	if (strlen(name)) {
		file.SetTo(ref, O_RDONLY);
		if (file.InitCheck() == B_NO_ERROR) {
			new_file = new BFile();
			if (make_file(&entry, new_file) == B_NO_ERROR) {
				entry.GetParent(&dir);
				bigtime_t creation = system_time();
				while (1) {
					strncpy(data, name, 240);
					sprintf(new_name, "%s:%Ld", data, creation++); 
					if (!dir.Contains(new_name))
						break;
				}
				entry.Rename(new_name);
				size = 16 * 1024;
				buffer = malloc(size);
				while (read = file.Read(buffer, size)) {
					new_file->Write(buffer, read);
				}
				while(file.GetNextAttrName(data) == B_NO_ERROR) {
					file.GetAttrInfo(data, &info);
					if (info.size > size) {
						size = info.size;
						buffer = realloc(buffer, size);
					}
					file.ReadAttr(data, info.type, 0, buffer, info.size);
					new_file->WriteAttr(data, info.type, 0, buffer, info.size);
				}
				free(buffer);
				node = new BNodeInfo(new_file);
				node->SetType(B_MAIL_TYPE);
				delete node;
		
				new_file->WriteAttr(B_MAIL_ATTR_RECIPIENTS, B_STRING_TYPE, 0, name, strlen(name) + 1);
				new_file->WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, "Pending", 8);
				flags = B_MAIL_PENDING | B_MAIL_SAVE;
				new_file->WriteAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &flags, sizeof(int32));
				delete new_file;
				if (now)
					result = send_queued_mail();
				else
					result = B_NO_ERROR;
			}
			else
				delete new_file;
		}
	}
	else
		result = B_MAIL_NO_RECIPIENT;
	free(name);
	return result;
}

//------------------------------------------------------------------

static void output64chunk(char *dst, int32 c1, int32 c2, int32 c3,
						  int32 pads)
{
	*dst++ = basis_64[c1 >> 2];
	*dst++ = basis_64[((c1 & 0x3) << 4) | ((c2 & 0xf0) >> 4)];
    if (pads == 2) {
		*dst++ = '=';
		*dst++ = '=';
    } else if (pads) {
		*dst++ = basis_64[((c2 & 0xf) << 2) | ((c3 & 0xc0) >>6)];
		*dst++ = '=';
    } else {
		*dst++ = basis_64[((c2 & 0xf) << 2) | ((c3 & 0xc0) >>6)];
		*dst++ = basis_64[c3 & 0x3f];
    }
}

//------------------------------------------------------------------

ssize_t decode_base64(char *dst, char *src, off_t size, bool is_text)
{
	bool	done = FALSE;
	bool	newline;
    int32	c1, c2, c3, c4;
	int32	index = 0;
	ssize_t	count = 0;

    while (index < size) {
		c1 = src[index++];
        if (isspace(c1)) {
            newline = (c1 == '\n');
            continue;
        }
        if (done)
			continue;
        newline = FALSE;
        while ((index < size) && isspace(c2 = src[index++]))
			;
        while ((index < size) && isspace(c3 = src[index++]))
			;
        while ((index < size) && isspace(c4 = src[index++]))
			;
        if (index == size)
            break;
        if ((c1 == '=') || (c2 == '=')) {
            done = TRUE;
            continue;
        }

        c1 = char64(c1);
        c2 = char64(c2);
		dst[count] = ((c1 << 2) | ((c2 & 0x30) >> 4));
		if ((is_text) && (dst[count] == '\r'))
			dst[count] = '\n';
		count++;
        if (c3 == '=')
            done = TRUE;
		else {
            c3 = char64(c3);
	    	dst[count] = (((c2 & 0xf) << 4) | ((c3 & 0x3C) >> 2));
			if ((is_text) && (dst[count] == '\r'))
				dst[count] = '\n';
			count++;
            if (c4 == '=')
                done = TRUE;
			else {
				c4 = char64(c4);
				dst[count] = (((c3 & 0x03) << 6) | c4);
				if ((is_text) && (dst[count] == '\r'))
					dst[count] = '\n';
				count++;
            }
        }
    }
	return count;
}

//------------------------------------------------------------------

ssize_t encode_base64(char *dst, char *src, off_t size)
{
    uchar		c1, c2, c3;
	int32		line = 0;
	off_t		index = 0;
	ssize_t		count = 0;

   	while (index != size) {
		c1 = src[index++];
    	if (index == size) {
           	output64chunk(&dst[count], c1, 0, 0, 2);
			count += 4;
		}
   		else {
			c2 = src[index++];
           	if (index == size) {
               	output64chunk(&dst[count], c1, c2, 0, 1);
				count += 4;
			}
           	else {
				c3 = src[index++];
      			output64chunk(&dst[count], c1, c2, c3, 0);
				count += 4;
			}
		}
       	line += 4;
		if (line > 71) {
			dst[count++] = '\r';
			dst[count++] = '\n';
			line = 0;
		}
	}
    if (line) {
		dst[count++] = '\r';
		dst[count++] = '\n';
	}
	return count;
}


//==================================================================
// member functions...

//------------------------------------------------------------------
		
BMailMessage::BMailMessage(void)
{
	fFields = new BList();
	fMultiPart = FALSE;
}

//------------------------------------------------------------------

BMailMessage::~BMailMessage(void)
{
	field		*field_list;

	while (field_list = (field *)fFields->FirstItem()) {
		empty_list(field_list->list);
		fFields->RemoveItem(field_list);
		delete field_list->list;
		free(field_list->name);
		free(field_list);
	}
	delete fFields;
}

//------------------------------------------------------------------

status_t BMailMessage::AddContent(const char *text, int32 len,
								  const char *encoding, bool clobber)
{
	return set_field(B_MAIL_BODY, CONTENT_TEXT, NULL, text, len, kNoConversion, encoding,
					 clobber);
}

//------------------------------------------------------------------

status_t BMailMessage::AddContent(const char *text, int32 len, uint32 encoding,
								  bool clobber)
{
	return set_field(B_MAIL_BODY, CONTENT_TEXT, NULL, text, len, encoding,
					 NULL, clobber);
}

//------------------------------------------------------------------

status_t BMailMessage::AddEnclosure(const char *path, bool clobber)
{
	fMultiPart = TRUE;
	return set_field(B_MAIL_BODY, CONTENT_ENCLOSURE, NULL, path,
					 strlen(path) + 1, kNoConversion, NULL, clobber);
}

//------------------------------------------------------------------

status_t BMailMessage::AddEnclosure(entry_ref *ref, bool clobber)
{
	BEntry		entry;
	BPath		path;
	status_t	result;

	entry.SetTo(ref);
	if ((result = entry.GetPath(&path)) == B_NO_ERROR) {
		fMultiPart = TRUE;
		result = set_field(B_MAIL_BODY, CONTENT_ENCLOSURE, NULL,
						   (char *)path.Path(), strlen(path.Path()) + 1,
						   kNoConversion, NULL, clobber);
	}
	return result;
}

//------------------------------------------------------------------

status_t BMailMessage::AddEnclosure(const char *mime_type, void *data,
									int32 len, bool clobber)
{
	fMultiPart = TRUE;
	return set_field(B_MAIL_BODY, CONTENT_ENCLOSURE, mime_type, data,
					 len, kNoConversion, NULL, clobber);
}

//------------------------------------------------------------------

status_t BMailMessage::AddHeaderField(uint32 encoding, const char *name, const char *data, 
									  bool clobber)
{
	return set_field(name, HEADER_STRING, NULL, data, strlen(data) + 1, encoding, NULL, clobber);
}

//------------------------------------------------------------------

status_t BMailMessage::AddHeaderField(const char *name, const char *data,
									  bool clobber)
{
	return AddHeaderField(kNoConversion, name, data, clobber);
}

//------------------------------------------------------------------

status_t BMailMessage::Send(bool now, bool remove)
{
	void				*data;
	bool				daemon = false;
	char				boundary[256];
	char				str[256];
	uint32				encoding = kNoConversion;
	char				*encoding_str = NULL;
	char				*last;
	char				*mime;
	char				*name;
	char				*text;
	int32				content_len;
	int32				dst_len;
	int32				field_count;
	int32				field_loop;
	int32				index;
	int32				header_len;
	int32				item_count;
	int32				item_loop;
	int32				len;
	int32				loop;
	int32				line_len;
	int32				src_len;
	BEntry				entry;
	BDirectory			dir;
	BFile				*file;
	BList				*list;
	attr_info			info;
	mail_pop_account	account;
	time_t				the_time;
	struct tm			*date;
	status_t			result;
	type_code			type;
	bool				needsQP = false;
	field_item			*stuff;
	char buf[NC_MAXVALLEN];
	struct passwd		*pwent;

// If there isn't a To or Bcc field, this isn't a valid rfc822 message

	if ((!count_fields(B_MAIL_TO)) && (!count_fields(B_MAIL_BCC)))
		return B_MAIL_NO_RECIPIENT;

	list = find_field(B_MAIL_FROM);
	if ((list) && (stuff = (field_item *)list->ItemAt(0)))
		daemon = !strcmp("mail_daemon", (char *)stuff->data);
	file = new BFile();
	if ((result = make_file(&entry, file, daemon)) == B_NO_ERROR) {

// Build up header...

		get_pop_account(&account);

		field_count = count_fields();
		for (field_loop = 0; field_loop < field_count; field_loop++) {
			get_field_name(&name, field_loop);
			if ((!cistrcmp(name, B_MAIL_BODY)) || (!cistrcmp(name, B_MAIL_BCC)))
				continue;
			line_len = strlen(name);
			file->Write(name, line_len);
			if (name[line_len - 1] != ' ') {
				file->Write(" ", 1);
				line_len++;
			}
			item_count = count_fields(name);
			for (item_loop = 0; item_loop < item_count; item_loop++) {
				bool dummy;

				find_field(name, &type, &mime, &data, &len, &encoding, &encoding_str, &dummy, item_loop);
				if (item_loop) {
					file->Write(",", 1);
					line_len++;
				}
				else {
					if (!cistrcmp(B_MAIL_PRIORITY, name))
						file->WriteAttr(B_MAIL_ATTR_PRIORITY, B_STRING_TYPE, 0, data, len);
					else if (!cistrcmp(B_MAIL_TO, name))
						file->WriteAttr(B_MAIL_ATTR_TO, B_STRING_TYPE, 0, data, len);
					else if (!cistrcmp(B_MAIL_CC, name))
						file->WriteAttr(B_MAIL_ATTR_CC, B_STRING_TYPE, 0, data, len);
					else if (!cistrcmp(B_MAIL_FROM, name))
						file->WriteAttr(B_MAIL_ATTR_FROM, B_STRING_TYPE, 0, data, len);
					else if (!cistrcmp(B_MAIL_SUBJECT, name))
						file->WriteAttr(B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0, data, len);
					else if (!cistrcmp(B_MAIL_REPLY, name))
						file->WriteAttr(B_MAIL_ATTR_REPLY, B_STRING_TYPE, 0, data, len);
					else if (!cistrcmp(B_MAIL_DATE, name)) {
						the_time = parsedate(name, time(NULL));
						if (the_time == -1)
							the_time = time(NULL);
						file->WriteAttr(B_MAIL_ATTR_WHEN, B_TIME_TYPE, 0,
										&the_time, sizeof(the_time));
					}
				}
				len--;
#if 0 // don't bother (rmp)
				if (line_len + len > 76) {
					file->Write("\r\n\t", 3);
					line_len = 0;
				}
#endif
				int32	theHeaderLen = len;
				char	*theHeader = encode_header(encoding, name, (char *)data, &theHeaderLen); 
				file->Write(theHeader, theHeaderLen);
				line_len += theHeaderLen;
				free(theHeader);
			}
			file->Write("\r\n", 2);
		}

// Add required fields if they don't already exist...

		if (!count_fields(B_MAIL_DATE)) {
			the_time = time(NULL);
			date = localtime(&the_time);
			strftime(str, 255, "%a, %d %b %Y %H:%M:%S %Z", date);
			write(file, B_MAIL_DATE, str);
			file->WriteAttr(B_MAIL_ATTR_WHEN, B_TIME_TYPE, 0, &the_time, sizeof(time_t));
		}

		if (!count_fields(B_MAIL_FROM)) {
			str[0] = 0;
			if (strlen(account.real_name))
				strcpy(boundary, account.real_name);
			else {
				pwent = getpwuid(getuid());
				if (pwent) {
					strncpy(boundary,pwent->pw_name,sizeof(boundary));
				}
			}
			if (boundary[0] != '"')
				sprintf(str, "\"%s\" <", boundary);
			else
				sprintf(str, "%s <", boundary);

			if (strlen(account.reply_to))
				strcat(str, account.reply_to);
			else {
				if (strlen(account.pop_name))
					strcpy(boundary, account.pop_name);
				else {
					gethostname(buf,NC_MAXVALLEN);
				}
				strcat(str, boundary);
				strcat(str, "@");
	
				if (strlen(account.pop_host)) {
					strcat(str, account.pop_host);
				}
			}
			strcat(str, ">");

			int32 theFromLen = strlen(str);
			// write attr as UTF-8
			file->WriteAttr(B_MAIL_ATTR_FROM, B_STRING_TYPE, 0, str, theFromLen + 1);
			// encode before actually writing as email content
			char *theFrom = encode_header(encoding, B_MAIL_FROM, str, &theFromLen);
			write(file, B_MAIL_FROM, theFrom);
			free(theFrom);
		}
  
// If not multipart but we have multiple text encodings, make it multipart...

		if (!fMultiPart) {
			last = NULL;
			item_count = count_fields(B_MAIL_BODY);
			for (item_loop = 0; item_loop < item_count; item_loop++) {
				bool itemNeedsQP;

				find_field(name, &type, &mime, &data, &len, &encoding, &encoding_str, &itemNeedsQP, item_loop);
				if (last) {
					if (cistrcmp(encoding_str, last)) {
						fMultiPart = TRUE;
						break;
					}
				}
				else
					last = encoding_str;	

				needsQP = (itemNeedsQP) ? true : needsQP;				
			}
		}

		file->Write("MIME-Version: 1.0\r\n", 19);
		file->WriteAttr(B_MAIL_ATTR_MIME, B_STRING_TYPE, 0, "1.0", 4);

		if (fMultiPart) {
			if (count_fields(B_MAIL_XMAILER)) {
				get_field_name(&name, 0);
				strncpy(str, name, sizeof(str) / 2);
			}
			else
				strcpy(str, MAIL_VERSION);
			index = 0;
			while ((str[index]) && (str[index] > ' ')) {
				index++;
			}
			str[index] = 0;
			sprintf(boundary, "_------_%s.rmp.%d_------_", str, time(NULL));
			sprintf(str, "multipart/mixed; \r\n\tboundary=\"%s\"", boundary);
			write(file, B_MAIL_CONTENT, str);
		}
		else {
			if (last) {
				sprintf(str, "text/plain; charset=\"%s\"", last);
				write(file, B_MAIL_CONTENT, str);
			}
			else
				write(file, B_MAIL_CONTENT, "text/plain; charset=\"ISO-8859-1\"");
			
			if (needsQP)
				write(file, B_MAIL_ENCODING, "quoted-printable");
		}

// Add Reply-To if there isn't one and user_info has one...
		if ((!count_fields(B_MAIL_REPLY)) && (strlen(account.reply_to))) {
			write(file, B_MAIL_REPLY, account.reply_to);
			file->WriteAttr(B_MAIL_ATTR_REPLY, B_STRING_TYPE, 0, account.reply_to,
											strlen(account.reply_to) + 1);
		}

// Add our X-Mailer if one hasn't been added...
		if (!count_fields(B_MAIL_XMAILER))
			write(file, B_MAIL_XMAILER, MAIL_VERSION);

// Add header/content separator...
		file->Write("\r\n", 2);

// Write header length as an attribute...
		header_len = file->Position();
		file->WriteAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &header_len,
											sizeof(header_len));

// Now construct the message content...

		if (fMultiPart)
			write(file, NULL, "This is a multi-part message in MIME format.");

		field_count = count_fields();
		for (field_loop = 0; field_loop < field_count; field_loop++) {
			get_field_name(&name, field_loop);
			if (cistrcmp(name, B_MAIL_BODY))
				continue;
			item_count = count_fields(name);
			for (item_loop = 0; item_loop < item_count; item_loop++) {
				bool itemNeedsQP;

				find_field(name, &type, &mime, &data, &len, &encoding, &encoding_str, &itemNeedsQP, item_loop);
				if (type == CONTENT_ENCLOSURE) {
					if (mime)
						add_data(file, boundary, mime, data, len);
					else
						add_file(file, boundary, (char *)data);
				}
				else if (len) {
					if (fMultiPart) {
						write(file, "--", boundary);
						sprintf(str, "text/plain; charset=\"%s\"", encoding_str);
						write(file, B_MAIL_CONTENT, str);
						if (itemNeedsQP)
							write(file, B_MAIL_ENCODING, "quoted-printable");
						file->Write("\r\n", 2);
					}

					text = (char *)data;
					if (text[0] == '.')
						file->Write(".", 1);
					index = 0;
					for (loop = 0; loop < len; loop++) {
						if (text[loop] == '\n') {
							if ((loop) && (text[loop - 1] != '\r')) {
								src_len = loop - index;
								dst_len = src_len;
								bool	malloced = false;
								char	*theText = text + index;
								if (itemNeedsQP)
									theText = qp_encode(theText, &dst_len, false, &malloced);
								file->Write(theText, dst_len);
								if (malloced)
									free(theText);
							}
							file->Write("\r", 1);
							file->Write(&text[loop], 1);
							if ((loop+1 < len) && (text[loop+1] == '.'))
								file->Write(".", 1);
							index = loop+1;
						}
					}
					if (index < len) {
						src_len = len - index;
						dst_len = src_len;
						bool	malloced = false;
						char	*theText = text + index;
						if (itemNeedsQP)
							theText = qp_encode(theText, &dst_len, false, &malloced);
						file->Write(theText, dst_len);
						if (malloced)
							free(theText);
						file->Write("\r\n", 2);
					}
				}
			}
		}
		if (fMultiPart) {
			file->Write("--", 2);
			write(file, boundary, "--");
		}

// Write content length as an attribute...
		content_len = file->Position() - header_len;
		file->WriteAttr(B_MAIL_ATTR_CONTENT, B_INT32_TYPE, 0, &content_len,
											sizeof(content_len));

// Concatinate all the recipients (tos', ccs' & bccs')

		name = (char *)malloc(0);
		index = 0;

		index = concatinate(&name, index, B_MAIL_TO);
		index = concatinate(&name, index, B_MAIL_CC);
		index = concatinate(&name, index, B_MAIL_BCC);
		name = (char *)realloc(name, index + 1);
		name[index] = 0;
		verify_recipients(&name);
		file->WriteAttr(B_MAIL_ATTR_RECIPIENTS, B_STRING_TYPE, 0, name, strlen(name) + 1);

		entry.GetParent(&dir);
		bigtime_t creation = system_time();
		while (1) {
			strncpy(boundary, name, 240);
			sprintf(str, "%s:%Ld", boundary, creation++); 
			if (!dir.Contains(str))
				break;
		}
		entry.Rename(str);
		free(name);

		// special case when mail_daemon is sending an error message
		if (daemon) {
			index = 0;
			file->WriteAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &index, sizeof(int32));
			file->WriteAttr(B_MAIL_ATTR_NAME, B_STRING_TYPE, 0, "mail_daemon", 12);
			file->WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, "New", 4);
			delete file;
			return B_NO_ERROR;
		}

		if (remove)
			index = B_MAIL_PENDING;
		else
			index = B_MAIL_PENDING | B_MAIL_SAVE;
		file->WriteAttr(B_MAIL_ATTR_FLAGS, B_INT32_TYPE, 0, &index, sizeof(int32));
		file->WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, "Pending", 8);
		delete (file);

		if (now)
			result = send_queued_mail();
		else
			result = B_NO_ERROR;
	}
	else
		delete file;
	return result;
}

//------------------------------------------------------------------

int32 BMailMessage::concatinate(char **name, int32 index, char *field)
{
	void		*data;
	uint32		encoding;
	char		*encoding_str;
	char		*mime;
	int32		item_count;
	int32		item_loop;
	int32		len;
	type_code	type;

	if (item_count = count_fields(field)) {
		for (item_loop = 0; item_loop < item_count; item_loop++) {
			bool dummy;

			find_field(field, &type, &mime, &data, &len, &encoding, &encoding_str, &dummy, item_loop);
			if (index) {
				*name = (char *)realloc(*name, index + 1);
				(*name)[index++] = ',';
			}
			len--;
			*name = (char *)realloc(*name, index + len);
			memcpy(&((*name)[index]), data, len);
			index += len;
		}
	}
	return index;
}

//------------------------------------------------------------------

int32 BMailMessage::count_fields(char *name)
{
	int32	count = 0;
	BList	*item;

	if (!name)
		count = fFields->CountItems();
	else if (item = find_field(name))
		count = item->CountItems();
	return count;
}

//------------------------------------------------------------------

BList* BMailMessage::find_field(const char *name)
{
	int32	index = 0;
	field	*field_list;

	while (field_list = (field *)fFields->ItemAt(index++)) {
		if (!cistrcmp((char *)name, field_list->name))
			return(field_list->list);
	}
	return NULL;
}

//------------------------------------------------------------------

status_t BMailMessage::find_field(char *name, type_code *type, char **mime,
								  void **data, int32 *len, uint32 *encoding, 
								  char **encoding_str, bool *needsQP, int32 index)
{
	BList		*item;
	field_item	*stuff;
	status_t	result = B_MAIL_UNKNOWN_FIELD;

	if (item = find_field(name)) {
		if (stuff = (field_item *)item->ItemAt(index)) {
			*type = stuff->type;
			*mime = stuff->mime_type;
			*data = stuff->data;
			*len = stuff->len;
			*encoding = stuff->encoding;
			*encoding_str = stuff->encoding_str;
			*needsQP = stuff->needsQP;
			result = B_NO_ERROR;
		}
		else
			result = B_BAD_INDEX;
	}
	return result;
}

//------------------------------------------------------------------

status_t BMailMessage::get_field_name(char **name, int32 index)
{
	field		*field_list;
	status_t	result;

	field_list = (field *)fFields->ItemAt(index);
	if (!field_list)
		result = B_BAD_INDEX;
	else {
		*name = field_list->name;
		result = B_NO_ERROR;
	}
	return result;
}

//------------------------------------------------------------------
void check_header(char **data, int32 *datalen)
{
	char buf[32000];
	char host[256];
	char *tok, *bufp, *hostp;
	bool atfound = false;
	bool modified = false;
	bool brace = false;
	mail_prefs_data host_data;
	
	
	memset(buf, 0, sizeof(buf));
	bufp = buf;
	memset(host, 0, sizeof(host));
	
	host[0] = 0;	
	get_mail_prefs_data(&host_data);
	strcpy(host, host_data.default_domain);
			
	if(host[0] == 0)
		get_smtp_host(host);
	
	if(host[0] == 0)
		return;
		
	hostp = host;	
	
	//
	// Append the default domain onto unqualified email addresses
	//
	// For this modification, we DO want either standalone names or names within < >
	// we DO NOT want names between () or "".  If angle brackets exist then
	// everything else in a given address is ignored.
	//
	tok = *data;

#define ADD_TO_BUF(x) if((bufp - buf) < sizeof(buf)) { *bufp = x; bufp++; } 

	while(*tok != 0)
	{
		// eat commas and white space
		
		while(*tok != 0 && strchr(" ,\t;\n", *tok) != 0)
		{
			ADD_TO_BUF(*tok);
			tok++;
		}
		if(*tok == '"')
		{
			ADD_TO_BUF(*tok);
			tok++;

			while(*tok != 0 && *tok != '"')
			{
				ADD_TO_BUF(*tok);
				tok++;
			}
			ADD_TO_BUF(*tok);
			tok++;
			continue;
		}
		if(*tok == '(')
		{
			ADD_TO_BUF(*tok);
			tok++;

			while(*tok != 0 && *tok != ')')
			{
				ADD_TO_BUF(*tok);
				tok++;
			}
			ADD_TO_BUF(*tok);
			tok++;
			continue;
		}
		
		if(strchr(tok, '<') != 0 && ((strchr(tok, ',') == 0) || (strchr(tok, '<') < strchr(tok, ','))))
		{
			//
		 	// addresses of the form "Joe Blow <joe@be.com>"
			//
			while(*tok != 0 && *tok != '<')
			{
				ADD_TO_BUF(*tok);
				tok++;
			}
			
		}
		//
		// once we are here, it is an address we need to check.  It may be enclosed in '<>'.
		//
		atfound = false;
		brace = false;
		while(*tok != 0 && strchr(" ,\t;\n", *tok) == 0)
		{
			if(*tok == '<')
			{
				brace = true;
			}
			ADD_TO_BUF(*tok);
			tok++;
			if(*tok == '@')
				atfound = true;		
		}
		if(atfound == false)
		{

			if(*(tok - 1) == '>')
			{
				bufp--;
				brace = true;
				if(*(bufp - 1) == '<')
					continue;
			}
			
			ADD_TO_BUF('@');
			modified = true;
			
			hostp = host;
			while(*hostp != 0)
			{
				ADD_TO_BUF(*hostp);
				hostp++;
			}		
			if(brace == true)
			{
				ADD_TO_BUF('>');
				brace = false;
			}
		}
	}
	
	if(modified == true)
	{
		free(*data);
		*datalen = strlen(buf) + 1;
		*data = (char *) calloc(*datalen + 1, 1);
		strcpy(*data, buf);
	}
}


status_t BMailMessage::set_field(const char *name, type_code type, const char *mime,
								 const void *data, int32 length, uint32 encoding,
								 const char* encoding_str, bool clobber)
{
	if (encoding >= (sizeof(kCharsets) / sizeof(kCharsets[0])))
		encoding = kNoConversion;
	
	char		*buffer;
	int32		src_len;
	int32		dst_len;
	BList		*item;
	field		*field_list;
	field_item	*stuff;
	status_t	result;

	
	if (!(item = find_field(name))) {
		field_list = (field *)malloc(sizeof(field));
		field_list->name = (char *)malloc(strlen(name) + 1);
		strcpy(field_list->name, name);
		field_list->list = new BList();
		item = field_list->list;
		fFields->AddItem(field_list);
	}
	else if (clobber)
		empty_list(item);

	stuff = (field_item *)malloc(sizeof(field_item));
	stuff->type = type;
	stuff->needsQP = false;
	stuff->encoding = encoding;
	stuff->encoding_str = NULL;
	if (type == CONTENT_TEXT) {
		if (encoding_str) {
			stuff->encoding = kNoConversion;
			stuff->encoding_str = (char *)malloc(strlen(encoding_str) + 1);
			strcpy(stuff->encoding_str, encoding_str);
			stuff->data = malloc(length);
			stuff->len = length;
			memcpy(stuff->data, data, length);
		}
		else {
			int32 convState = 0;
			int32 bufLen = length * 3;

			if (encoding == kNoConversion) {
				dst_len = length;
				buffer = (char *)malloc(dst_len + 1);
				memcpy(buffer, data, dst_len);
				buffer[dst_len] = '\0';
			}
			else {
				buffer = (char *)malloc(bufLen);
				src_len = length;
				dst_len = bufLen;
				if ((result = convert_from_utf8(encoding, (char *)data, &src_len,
												buffer, &dst_len, &convState)) != B_NO_ERROR) {
					free(stuff);
					free(buffer);
					return result;
				}
	
				if (encoding == B_JIS_CONVERSION)
					dst_len += force_kanjiout(buffer + dst_len, bufLen - dst_len, &convState);

				stuff->encoding_str = (char *)malloc(strlen(kCharsets[encoding]) + 1);
				strcpy(stuff->encoding_str, kCharsets[encoding]);
			}

			stuff->data = buffer;
			stuff->len = dst_len;
			stuff->needsQP = is_8bit_data(buffer, dst_len);
		}
	}
	else {
		stuff->encoding = encoding;
		stuff->encoding_str = NULL;
		stuff->data = malloc(length);
		stuff->len = length;
		memcpy(stuff->data, data, length);
	}
	if (mime) {
		stuff->mime_type = (char *)malloc(strlen(mime) + 1);
		strcpy(stuff->mime_type, mime);
	}
	else
		stuff->mime_type = NULL;
		
	if(type == HEADER_STRING && (!strcmp(name, B_MAIL_TO)
								|| !strcmp(name, B_MAIL_FROM)
								|| !strcmp(name, B_MAIL_CC)
								|| !strcmp(name, B_MAIL_BCC)))
	{
		check_header((char **) &stuff->data, &stuff->len);
	}

	item->AddItem(stuff);

	return B_NO_ERROR;
}


//==================================================================
// c functions...

static void add_data(BFile *file, char *boundary, char *mime, void *data, int32 len)
{
	char	*buffer;
	ssize_t	size;

	write(file, "\r\n--", boundary);
	write(file, B_MAIL_CONTENT, mime);
	write(file, B_MAIL_ENCODING, "base64\r\n");

	buffer = (char *)malloc(len * 2);
	size = encode_base64(buffer, (char *)data, len);
	file->Write(buffer, size);
	free(buffer);
}

//------------------------------------------------------------------

static status_t add_file(BFile *file, char *boundary, char *path)
{
	char		*buffer;
	char		*data;
	char		bound[256];
	char		file_name[B_FILE_NAME_LENGTH];
	char		name[B_FILE_NAME_LENGTH];
	uint32		tmp;
	BEntry		entry;
	BFile		enclosure;
	BNodeInfo	*info;
	status_t	result;
	attr_info	attribute;
	off_t		bytes;
	off_t		len;
	ssize_t		size;

	enclosure.SetTo(path, O_RDONLY);
	if ((result = enclosure.InitCheck()) == B_NO_ERROR) {
		write(file, "--", boundary);
		entry.SetTo(path);
		entry.GetName(file_name);
		sprintf(bound, "++++++BFile%Ld++++++", system_time());
		file->Write(B_MAIL_CONTENT, strlen(B_MAIL_CONTENT));
		file->Write("multipart/x-bfile; \r\n\tboundary=\"", 32);
		file->Write(bound, strlen(bound));
		// ** Changed by Kenny. This is the declaration for the outer boundary.
		// Including the name of the attachment here was confusing Netscape
		// Communicator causing Communicator to screw up the mime parsing.
		// Removing the name field here solved the problem. This doesn't effect
		// other mail clients since the name field is still included lower down...
//		file->Write("\"; \r\n\tname=\"", 12);
//		file->Write(file_name, strlen(file_name));
		file->Write("\"\r\n\r\n", 5);
		
		info = new BNodeInfo(&enclosure);
		if (info->GetType(name) != B_NO_ERROR)
			strcpy(name, "application/octet-stream");
		delete info;
		write(file, "--", bound);
		file->Write(B_MAIL_CONTENT, strlen(B_MAIL_CONTENT));
		file->Write(name, strlen(name));
		file->Write("; \r\n\tname=\"", 11);
		file->Write(file_name, strlen(file_name));
		file->Write("\"\r\n", 3);
		write(file, B_MAIL_ENCODING, "base64\r\n");

		enclosure.GetSize(&len);
		(len <= 65535) ? bytes = len : bytes = 65535;
		data = (char *)malloc(bytes);
		buffer = (char *)malloc(bytes * 2);
		do {
			enclosure.Read(data, bytes);
			size = encode_base64(buffer, data, bytes);
			file->Write(buffer, size);
			len -= bytes;
			if (len < bytes)
				bytes = len;
		} while (len);
		free(buffer);
		free(data);

		data = (char *)malloc(0);
		size = 0;
		while(enclosure.GetNextAttrName(name) == B_NO_ERROR) {
			enclosure.GetAttrInfo(name, &attribute);
			data = (char *)realloc(data, size + attribute.size + strlen(name) + 1 +
								   sizeof(attribute.size) + sizeof(attribute.type));
			memcpy(&data[size], name, strlen(name) + 1);
			size += strlen(name) + 1;
			tmp = B_HOST_TO_BENDIAN_INT32(attribute.type);
			memcpy(&data[size], &tmp, sizeof(attribute.type));
			size += sizeof(attribute.type);
			len = B_HOST_TO_BENDIAN_INT64(attribute.size);
			memcpy(&data[size], &len, sizeof(attribute.size));
			size += sizeof(attribute.size);
			enclosure.ReadAttr(name, attribute.type, 0, &data[size], attribute.size);
			swap_data(attribute.type, &data[size], attribute.size, B_SWAP_HOST_TO_BENDIAN);
			size += attribute.size;
		}
		write(file, "--", bound);
		file->Write(B_MAIL_CONTENT, strlen(B_MAIL_CONTENT));
		// ** Changed by Kenny. Since non-BeOS mail clients see the attribute as a seperate
		// attachment, we should give it a more meaningful name. Otherwise, the user
		// will see two attachments with the same name. This doesn't apply to BeOS
		// mail clients...
//		file->Write("application/x-be_attribute; name=\"", 34);
//		file->Write(file_name, strlen(file_name));
		file->Write("application/x-be_attribute; \r\n\tname=\"BeOS Attributes", 52);
		file->Write("\"\r\n", 3);
		write(file, B_MAIL_ENCODING, "base64\r\n");

		buffer = (char *)malloc(size * 2);
		size = encode_base64(buffer, data, size);
		file->Write(buffer, size);
		free(buffer);
		free(data);
		file->Write("--", 2);
		write(file, bound, "--");
		result = B_NO_ERROR;
	}
	return result;
}

//------------------------------------------------------------------

static int32 cistrcmp(const char *str1, const char *str2)
{
	char	c1;
	char	c2;
	int32	len;
	int32	loop;

	len = strlen(str1) + 1;
	for (loop = 0; loop < len; loop++) {
		c1 = str1[loop];
		if ((c1 >= 'A') && (c1 <= 'Z'))
			c1 += ('a' - 'A');
		c2 = str2[loop];
		if ((c2 >= 'A') && (c2 <= 'Z'))
			c2 += ('a' - 'A');
		if (c1 == c2) {
		}
		else if (c1 < c2)
			return -1;
		else if ((c1 > c2) || (!c2))
			return 1;
	}
	return 0;
}

//------------------------------------------------------------------

static void empty_list(BList *list)
{
	field_item	*stuff;

	while (stuff = (field_item *)list->FirstItem()) {
		list->RemoveItem(stuff);
		free(stuff->data);
		if (stuff->encoding_str)
			free(stuff->encoding_str);
		if (stuff->mime_type)
			free(stuff->mime_type);
		free(stuff);
	}
}

//------------------------------------------------------------------

#include <stdio.h>
#include <errno.h>

static status_t make_file(BEntry *item, BFile *file, bool in)
{
	char		box[B_FILE_NAME_LENGTH];
	char		name[B_FILE_NAME_LENGTH];
	status_t	result;
	BDirectory	dir;
	BEntry		entry;
	BNodeInfo	*node;
	BPath		path;

	if ((result = find_directory(B_USER_DIRECTORY, &path)) != B_NO_ERROR)
		return result;
	dir.SetTo(path.Path());
	if ((result = dir.InitCheck()) != B_NO_ERROR)
		return result;

	if ((result = dir.FindEntry("mail", &entry)) != B_NO_ERROR) {
		if ((result = dir.CreateDirectory("mail", &dir)) != B_NO_ERROR)
			return result;
	}
	else {
		dir.SetTo(&entry);
		if ((result = dir.InitCheck()) != B_NO_ERROR)
			return result;
	}

	(in) ? strcpy(box, "in") : strcpy(box, "out");
	if ((result = dir.FindEntry(box, &entry)) != B_NO_ERROR) {
		if ((result = dir.CreateDirectory(box, &dir)) != B_NO_ERROR)
			return result;
	}
	else {
		dir.SetTo(&entry);
		if ((result = dir.InitCheck()) != B_NO_ERROR)
			return result;
	}
		
	while (1) {
		int ret;
		sprintf(name, "email_%d", file_index++);
		ret = dir.CreateFile(name, file, TRUE);

		if (ret != B_FILE_EXISTS && ret != B_OK)  /* any other error should break us out */
			return ret;
	
		if (ret == B_OK) {
			node = new BNodeInfo(file);
			node->SetType(B_MAIL_TYPE);
			delete node;
			dir.FindEntry(name, item);
			break;
		}
	}
	return B_NO_ERROR;
}

//------------------------------------------------------------------

static void verify_recipients(char **to)
{
	bool		quote;
	char		host[B_MAX_HOST_NAME_LENGTH];
	char		*dst;
	char		*src;
	char		*name;
	int32		dst_len = 0;
	int32		index = 0;
	int32		loop;
	int32		offset;
	int32		len;
	int32		l;
	int32		src_len;

	src = *to;
	src_len = strlen(src);
	while (index < src_len) {
		len = 0;
		quote = FALSE;
		while ((src[index + len]) && ((quote) || ((!quote) && (src[index + len] != ',')))) {
			if (src[index + len] == '"')
				quote = !quote;
			len++;
		}

// remove quoted text...
		if (len) {
			offset = index;
			index += len + 1;
			for (loop = offset; loop < offset + len; loop++) {
				if (src[loop] == '"') {
					len -= (loop - offset) + 1;
					offset = loop + 1;
					while ((len) && (src[offset] != '"')) {
						offset++;
						len--;
					}
					
					if( len )
					{
						offset++;
						len--;
					}
					break;
				}
			}

// look for bracketed '<...>' text...
			for (loop = offset; loop < offset + len; loop++) {
				if (src[loop] == '<') {
					offset = loop + 1;
					l = 0;
					while ((l < len) && (src[offset + l] != '>')) {
						l++;
					}
					goto add;
				}
			}

// not bracketed? then take it all...
			l = len;
			while ((l) && ((src[offset] == ' ') || (src[offset] == '\t'))) {
				offset++;
				l--;
			}

add:
			if (l) {
				if (dst_len) {
					dst = (char *)realloc(dst, dst_len + 3 + l);
					memcpy(&dst[dst_len], ",<", 2);
					dst_len += 2;
				}
				else {
					dst = (char *)malloc(2 + l);
					dst[0] = '<';
					dst_len++;
				}
				memcpy(&dst[dst_len], &src[offset], l);
				dst_len += l;
				dst[dst_len++] = '>';
			}
		}
		else
			index++;
	}
	if (dst_len) {
		dst = (char *)realloc(dst, dst_len + 1);
		dst[dst_len] = 0;
	}
	else {
		dst = (char *)malloc(1);
		dst[0] = 0;
	}
	free(*to);
	*to = dst;

	if ((get_smtp_host(host) != B_NO_ERROR) || (!strlen(host))) {
		beep();
		if ((new BAlert("",
				"E-mail can not be sent until an SMTP host is configured.",
				"Queue", "Configure"))->Go())
			be_roster->Launch("application/x-vnd.Be-mprf", 1, NULL);
	}
}

//------------------------------------------------------------------

static ssize_t write(BFile *file, char *label, char *setting)
{
	ssize_t		len = 0;

	if (label)
		len += file->Write(label, strlen(label));
	if (setting)
		len += file->Write(setting, strlen(setting));
	len += file->Write("\r\n", 2);
	return len;
}

//------------------------------------------------------------------

static bool is_8bit_data(const char *string, int32 stringLen)
{
	for (int32 i = 0; i < stringLen; i++) {
		if (((uchar)string[i]) > 0x7F)
			return (true);
	}

	return (false);
}

//------------------------------------------------------------------

static char *qp_encode(const char *string, int32 *stringLen, bool strict, bool *malloced)
{
	int32	origStringLen = *stringLen;
	int32	numQPChars = 0;

	for (int32 i = 0; i < origStringLen; i++) {
		if (needs_qp_encode(string[i], strict))
			numQPChars++;
	}

	if (numQPChars < 1) {
		*malloced = false;
		return ((char *)string);
	}

	*stringLen = origStringLen + (numQPChars * 2);
	char *result = (char *)malloc(*stringLen + 1);

	int32 resultOffset = 0;
	for (int32 i = 0; i < origStringLen; i++) {
		uchar theChar = (uchar)string[i];

		if (needs_qp_encode(theChar, strict)) {
			char hex[3];
			sprintf(hex, "%02X", theChar);

			result[resultOffset++] = '=';
			result[resultOffset++] = hex[0];
			result[resultOffset++] = hex[1];
		}
		else {
			if ((strict) && (theChar == ' '))
				theChar = '_';
				
			result[resultOffset++] = theChar;
		}
	}

	*malloced = true;
	result[*stringLen] = '\0';
	return (result);	
}

//------------------------------------------------------------------

static int32 b64_encode(char *dst, const char *src, int32 srcLen)
{
	int32 i = 0;
	int32 dstLen = 0;
	dst[dstLen] = '\0';

	while (i < srcLen) {
		switch (srcLen - i) {
			case 2:
			{
				uchar a = src[i];
				uchar b = src[i + 1];

				dst[dstLen++] = basis_64[a >> 2];
				dst[dstLen++] = basis_64[((a & 3) << 4) + (b >> 4)];
				dst[dstLen++] = basis_64[(b & 15) << 2];
				dst[dstLen++] = '=';
				i += 2;
				break;
			}

			case 1:	
			{
				uchar a = src[i];

				dst[dstLen++] = basis_64[a >> 2];
				dst[dstLen++] = basis_64[(a & 3) << 4];
				dst[dstLen++] = '=';
				dst[dstLen++] = '=';
				i += 1;			
				break;
			}

			default:
			{
				uchar a = src[i];
				uchar b = src[i + 1];
				uchar c = src[i + 2];

				dst[dstLen++] = basis_64[a >> 2];
				dst[dstLen++] = basis_64[((a & 3) << 4) + (b >> 4)];
				dst[dstLen++] = basis_64[((b & 15) << 2) + (c >> 6)];
				dst[dstLen++] = basis_64[c & 63];
				i += 3;
				break;
			}
		}
	}

	dst[dstLen] = '\0';
	return (dstLen);
}	

//------------------------------------------------------------------

static char *encode_header(uint32 encoding, const char *header, const char *string, int32 *stringLen)
{
	if ((encoding == kNoConversion) || (!is_8bit_data(string, *stringLen))) {
		char *result = (char *)malloc(*stringLen + 1);
		memcpy(result, string, *stringLen);
		result[*stringLen] = '\0';
		return (result);
	}

	const char	*data = string;
	int32		dataLen = *stringLen;
	char		*theHeader = (char *)malloc(1);
	int32		theHeaderLen = 0;
	theHeader[theHeaderLen] = '\0';

	bool parseForRecipients = (cistrcmp(B_MAIL_TO, header) == 0) || (cistrcmp(B_MAIL_CC, header) == 0) ||
							  (cistrcmp(B_MAIL_BCC, header) == 0) || (cistrcmp(B_MAIL_FROM, header) == 0) ||
							  (cistrcmp(B_MAIL_REPLY, header) == 0);

	int32 offset = 0;
	while (offset < dataLen) {
		int32	origOffset = offset;
		int32	bracketedLen = 0;
		bool	foundConvertee = false;

		if (!parseForRecipients) {
			offset = dataLen;
			foundConvertee = true;
		}
		else { 
			// find the length of the current comma-delimited recipient
			for ( ; offset < dataLen; offset++) {
				if (data[offset] == ',') {
					// include the ',' and any whitespace
					for (offset++; offset < dataLen; offset++) {
						if (!isspace(data[offset]))
							break;
					}
					break;
				}
			}
		}
		
		const char	*convertee = data + origOffset;
		int32		converteeLen = offset - origOffset;

		if (parseForRecipients) {
			// look for either a '<' or a '('
			for (int32 i = 0; i < converteeLen; i++) {
				uchar theChar = convertee[i];
	
				if (theChar == '<') {
					// look for a '>'
					for (bracketedLen = 1; i + bracketedLen < converteeLen; bracketedLen++) {
						if (convertee[i + bracketedLen] == '>') {
							// include '>' and whitespace in the count
							for (bracketedLen++; i + bracketedLen < converteeLen; bracketedLen++) {
								if (!isspace(convertee[i + bracketedLen]))
									break;
							}
							foundConvertee = true;
							break;
						}
					}
	
					// look for the last non-whitespace char before the '<'
					for (int32 j = i - 1; j >= 0; j--) {
						if (!isspace(convertee[j])) {
							converteeLen = j + 1;
							break;
						}
						
						// include whitespace before '<' as part of bracketed text
						bracketedLen++;
					}
					break;	
				}
				else {
					if (theChar == '(') {
						convertee += i + 1;
						converteeLen -= i + 1;

						// look for a ')'		
						for (int32 j = 0; j < converteeLen; j++) {
							if (convertee[j] == ')') {
								if ((j > 0) && (convertee[j - 1] == '\\'))
									// escaped ')', ignore it
									continue;
								else {
									converteeLen = j;
									foundConvertee = true;
									break;
								}
							}
						}
						break;
					}
				}
			}	
		}

		if (!foundConvertee) {
			// nothing needs to be converted
			convertee = data + origOffset;
			converteeLen = 0;
			bracketedLen = 0;
		}

		// copy over to header any text that exists before the convertee
		const char	*leftText = data + origOffset;
		int32		leftTextLen = convertee - leftText;
		if (leftTextLen > 0)
			theHeader = concat_str(theHeader, &theHeaderLen, leftText, leftTextLen);

		// convert convertee from UTF-8, rfc2047-ize it, and copy over to header
		if (converteeLen > 0) {
			int32	rfc2047Len = converteeLen;
			char	*rfc2047 = utf8_to_rfc2047(encoding, convertee, &rfc2047Len);
			if (rfc2047Len > 0)
				theHeader = concat_str(theHeader, &theHeaderLen, rfc2047, rfc2047Len);
			free(rfc2047);
		}
		
		// copy over to header any text that exists after the convertee
		if (bracketedLen < 1) {
			// do a straight copy
			const char	*rightText = leftText + leftTextLen + converteeLen;
			int32		rightTextLen = (data + offset) - rightText;
			if (rightTextLen > 0)
				theHeader = concat_str(theHeader, &theHeaderLen, rightText, rightTextLen);
		}
		else {
			// do a straight copy of the bracketed text
			const char *bracketedText = leftText + leftTextLen + converteeLen;
			theHeader = concat_str(theHeader, &theHeaderLen, bracketedText, bracketedLen);

			// find text after the '>' so that we can convert it as well
			const char	*rightText = bracketedText + bracketedLen;			
			int32		origRightTextLen = (data + offset) - rightText;
			int32		rightTextLen = origRightTextLen;

			// look for the ',', don't want to convert it (or anything after it)
			for ( ; rightTextLen > 0; rightTextLen--) {
				if (rightText[rightTextLen - 1] == ',') {
					// include the ','
					rightTextLen--;
					break;
				}
			}

			// convert leftover text
			if (rightTextLen > 0) {
				int32	rfc2047Len = rightTextLen;
				char	*rfc2047 = utf8_to_rfc2047(encoding, rightText, &rfc2047Len);
				if (rfc2047Len > 0) 
					theHeader = concat_str(theHeader, &theHeaderLen, rfc2047, rfc2047Len);
				free(rfc2047);
			}

			// copy over (without converting) the ',' and whitespace
			const char	*commaText = rightText + rightTextLen;
			int32		commaTextLen = origRightTextLen - rightTextLen;
			if (commaTextLen > 0) 
				theHeader = concat_str(theHeader, &theHeaderLen, commaText, commaTextLen);						
		}
	}
	
	*stringLen = theHeaderLen;
	return (theHeader);
}

//------------------------------------------------------------------

static char *utf8_to_rfc2047(uint32 encoding, const char *string, int32 *stringLen)
{
	char	*rfc2047 = (char *)malloc(1);
	int32	rfc2047Len = 0;
	rfc2047[rfc2047Len] = '\0';

	int32	convState = 0;
	int32	recipientConv = *stringLen;
	int32	bufferSize = *stringLen * 3;
	int32	convRecipientLen = bufferSize;
	char	*convRecipient = (char *)malloc(bufferSize + 1);

	convert_from_utf8(encoding, string, &recipientConv, 
					  convRecipient, &convRecipientLen, &convState);
	convRecipient[convRecipientLen] = '\0';

	if (encoding == B_JIS_CONVERSION) {
		convRecipientLen += force_kanjiout(convRecipient + convRecipientLen, 
										   bufferSize - convRecipientLen, 
										   &convState);
		convRecipient[convRecipientLen] = '\0';

		// rfc2047-ize only the kanji portion
		const char *kKanjiIn = "\033$B";
		const int32	kKanjiInLen = strlen(kKanjiIn);
		const char *kKanjiOut = "\033(J";
		const int32	kKanjiOutLen = strlen(kKanjiOut);

		int32 offset = 0;
		while (offset < convRecipientLen) {
			const char *curConv = convRecipient + offset;

			const char *kanjiIn = strstr(curConv, kKanjiIn);
			if (kanjiIn == NULL) {
				rfc2047 = concat_str(rfc2047, &rfc2047Len, curConv, convRecipientLen - offset);
				break;
			}
			else {
				const char *kanjiOut = strstr(kanjiIn + kKanjiInLen, kKanjiOut);
				kanjiOut = (kanjiOut == NULL) ? convRecipient + convRecipientLen - kKanjiOutLen : kanjiOut;

				rfc2047 = concat_str(rfc2047, &rfc2047Len, curConv, kanjiIn - curConv);
			
				char	*encodedText = (char *)malloc(convRecipientLen * 2);
				int32	encodedTextLen = b64_encode(encodedText, kanjiIn, kanjiOut + kKanjiOutLen - kanjiIn);
		
				//            =?               %s               ?B?        %s         ?=
				int32 escLen = 2 + strlen(kCharsets[encoding]) + 3 + encodedTextLen + 2;
				rfc2047 = (char *)realloc(rfc2047, rfc2047Len + escLen + 1);
				sprintf(rfc2047 + rfc2047Len, "=?%s?B?%s?=", kCharsets[encoding], encodedText);				
				rfc2047Len += escLen;
				free(encodedText);	

				offset += (kanjiOut + kKanjiOutLen) - curConv;
			}
		}
	}
	else {
		if (!is_8bit_data(convRecipient, convRecipientLen)) {
			rfc2047Len = convRecipientLen;
			rfc2047 = (char *)realloc(rfc2047, rfc2047Len + 1);
			memcpy(rfc2047, convRecipient, rfc2047Len);
			rfc2047[rfc2047Len] = '\0';
		}
		else {
			int32	encodedTextLen = convRecipientLen;
			bool	encMalloced = false;
			char	*encodedText = qp_encode(convRecipient, &encodedTextLen, true, &encMalloced);
	
			//          =?               %s               ?Q?        %s         ?=
			rfc2047Len = 2 + strlen(kCharsets[encoding]) + 3 + encodedTextLen + 2;
			rfc2047 = (char *)realloc(rfc2047, rfc2047Len + 1);
			sprintf(rfc2047, "=?%s?Q?%s?=", kCharsets[encoding], encodedText);
			if (encMalloced)
				free(encodedText);
		}
	}

	free(convRecipient);
	*stringLen = rfc2047Len;
	return (rfc2047);	
}

//------------------------------------------------------------------

static char *concat_str(char *dst, int32 *dstLen, const char *src, int32 srcLen)
{
	dst = (char *)realloc(dst, *dstLen + srcLen + 1);
	memcpy(dst + *dstLen, src, srcLen);
	*dstLen += srcLen;
	dst[*dstLen] = '\0';

	return (dst);
}

//------------------------------------------------------------------

static int32 force_kanjiout(char *dst, int32 dstLen, int32 *convState)
{
	const char	*kKanjiOut = " ";
	const int32	kKanjiOutLength = 1;
	int32		kanjiOutLength = kKanjiOutLength;
	int32		convertedKanjiOut = dstLen;

	convert_from_utf8(B_JIS_CONVERSION, kKanjiOut, &kanjiOutLength, 
					  dst, &convertedKanjiOut, convState);

	int32 result = convertedKanjiOut - kKanjiOutLength;
	return ((result < 0) ? 0 : result);
}
