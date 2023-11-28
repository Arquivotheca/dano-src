//--------------------------------------------------------------------
//	
//	mail_parser.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "mail_parser.h"

#include <support/BufferIO.h>

int32	result = B_NO_ERROR;


//====================================================================

int32 readline(BPositionIO *file, char *buffer, int32 len)
{
	int32	cnt = 0;
	int32	found;
	
	while (cnt < len) {
		found = file->Read(buffer+cnt, 32);
		if (found < 0)
			return found;
		if (found == 0)
			return cnt;
		while (found > 0) {
			found--;
			if (buffer[cnt] == '\n') {
				if (found > 0) file->Seek(-found, SEEK_CUR);
				return cnt + 1;
			}
			cnt++;
		}
	}
	return cnt;
}

/*-----------------------------------------------------------------*/

int32 compare(char *one, const char *two, int32 len, bool use_case)
{
	char	n;
	int32	loop;

	if (use_case)
		return(memcmp((void *)one, (void *)two, len));
	else {
		for (loop = 0; loop < len; loop++) {
			n = one[loop];
			if ((n >= 'A') && (n <= 'Z'))
				n += ('a' - 'A');
			if (n < two[loop])
				return(-1);
			else if (n > two[loop])
				return(1);
		}
		return(0);
	}
}


//====================================================================

int main(int argc, char **argv)
{
	TMailParser	app;

	app.Run();
	return result;
}


//====================================================================

TMailParser::TMailParser(void)
			:BApplication("application/x-vnd.Be-mprs")
{
}


//--------------------------------------------------------------------

void TMailParser::ArgvReceived(int32 argc, char **argv)
{
	int32		loop;
	BEntry		entry;

	for (loop = 1; loop < argc; loop++) {
		entry.SetTo(argv[loop]);
		Parse(&entry);
	}
}

//--------------------------------------------------------------------

void TMailParser::RefsReceived(BMessage *msg)
{
	int32		i = 0;
	BEntry		entry;
	entry_ref	ref;

	while (msg->HasRef("refs", i)) {
		msg->FindRef("refs", i++, &ref);
		entry.SetTo(&ref);
		Parse(&entry);
	}
}

//--------------------------------------------------------------------

void TMailParser::ReadyToRun(void)
{
	PostMessage(B_QUIT_REQUESTED);
}

//--------------------------------------------------------------------

status_t TMailParser::Parse(BEntry *entry)
{
	char		byte;
	char		line[1024];
	char		name[B_FILE_NAME_LENGTH];
	char		new_name[B_FILE_NAME_LENGTH];
	char		*bufpos;
	int32		content;
	int32		cnt;
	int32		i = 0;
	int32		index;
	int32		len;
	int32		loop;
	int32		offset;
	int32		size;
	int32		lines = 0;
	time_t		now;
	time_t		when;
	BDirectory	dir;
	BFile		file;
	BMessage	message(B_REFS_RECEIVED);
	BMessenger	tracker("application/x-vnd.Be-TRAK", -1, NULL);
	BNodeInfo	*node;
	entry_ref	ref;
	struct tm	date;
	status_t	result;

	file.SetTo(entry, O_RDWR);
	if ((result = file.InitCheck()) == B_NO_ERROR) {
		BBufferIO buffer(&file, 65536, false);
		now = time((time_t *)NULL);
		offset = 0;
		content = 0;
		name[0] = 0;
		when = 0;

		for (;;) {
			len = readline(&buffer, line, sizeof(line));
			if ((len <= 0) || ((line[0] == '.') && (line[1] == '\r') &&
												   (line[2] == '\n'))) {
				if (content) {
					len = offset - content;
					if (len < 0)
						len = 0;
				}
				else
					len = 0;
				offset -= len;
	
				file.WriteAttr(B_MAIL_ATTR_HEADER, B_INT32_TYPE, 0, &offset, sizeof(int32));
				file.WriteAttr(B_MAIL_ATTR_CONTENT, B_INT32_TYPE, 0, &len, sizeof(int32));
				
				if (strlen(name)) {
					index = 0;
					while (name[index]) {
						if (name[index] == '/')
							name[index] = '_';
						index++;
					}
					if (when) {
						date = *localtime(&when);
						strftime(line, 16, "%m%d%y%H%M%S", &date);
						sprintf(new_name, "%s_%s", name, line);
					}
					else
						strcpy(new_name, name);
					index = 1;
					entry->GetParent(&dir);
					while (1) {
						sprintf(line, "%s:%d", new_name, index++);
						if (!dir.Contains(line))
							break;
					}
					//entry->Rename(line);
	
					node = new BNodeInfo(&file);
					node->SetType(B_MAIL_TYPE);

					if (tracker.IsValid()) {
						entry->GetRef(&ref);
						message.AddRef("refs", &ref);
						tracker.SendMessage(&message);
					}

					delete node;
				}
				file.WriteAttr(B_MAIL_ATTR_STATUS, B_STRING_TYPE, 0, "New", 4);
				break;
			}
	
			loop = 0;
			bufpos = line;
			if (!content) {
				if (len == 2)
					content = offset + 2;
				size = strsize(FIELD_FROM);
				if (!compare(bufpos, FIELD_FROM, size - 1, FALSE)) {
					bufpos[len - 2] = 0;
					file.WriteAttr(B_MAIL_ATTR_FROM, B_STRING_TYPE, 0,
						&bufpos[size], strlen(&bufpos[size]) + 1);
					index = 0;
					cnt = max_c(B_FILE_NAME_LENGTH - 1, len);
					for (loop = size; loop < cnt; loop++) {
						byte = bufpos[loop];
						if ((!index) && ((byte == ' ') || (byte == '\t')))
							continue;
						if ((loop != size) && (byte == '<') &&
							(bufpos[loop - 1] == ' ')) {
							if (index)
								name[index - 1] = 0;
							else
								name[0] = 0;
							break;
						}
						if ((byte != '"') && (byte != '\''))
							name[index++] = byte;
					}
					file.WriteAttr(B_MAIL_ATTR_NAME, B_STRING_TYPE, 0,
						name, strlen(name) + 1);
					bufpos[len - 2] = '\r';
				}
	
				size = strsize(FIELD_TO);
				if (!compare(bufpos, FIELD_TO, size - 1, FALSE)) {
					bufpos[len - 2] = 0;
					file.WriteAttr(B_MAIL_ATTR_TO, B_STRING_TYPE, 0,
						&bufpos[size], strlen(&bufpos[size]) + 1);
					bufpos[len - 2] = '\r';
				}
	
				size = strsize(FIELD_REPLY);
				if (!compare(bufpos, FIELD_REPLY, size - 1, FALSE)) {
					bufpos[len - 2] = 0;
					file.WriteAttr(B_MAIL_ATTR_REPLY, B_STRING_TYPE, 0,
						&bufpos[size], strlen(&bufpos[size]) + 1);
					bufpos[len - 2] = '\r';
				}
	
				size = strsize(FIELD_SUBJECT);
				if (!compare(bufpos, FIELD_SUBJECT, size - 1, FALSE)) {
					bufpos[len - 2] = 0;
					file.WriteAttr(B_MAIL_ATTR_SUBJECT, B_STRING_TYPE, 0,
						&bufpos[size], strlen(&bufpos[size]) + 1);
					bufpos[len - 2] = '\r';
				}
	
				size = strsize(FIELD_WHEN);
				if (!compare(bufpos, FIELD_WHEN, size - 1, FALSE)) {
					bufpos[len - 2] = 0;
					when = parsedate(&bufpos[size],
										time((time_t *)NULL));
					if (when == -1)
						when = 0;
					file.WriteAttr(B_MAIL_ATTR_WHEN, B_TIME_TYPE, 0,
						&when, sizeof(when));
					bufpos[len - 2] = '\r';
				}
	
				size = strsize(FIELD_PRIORITY);
				if (!compare(bufpos, FIELD_PRIORITY, size - 1, FALSE)) {
					bufpos[len - 2] = 0;
					file.WriteAttr(B_MAIL_ATTR_PRIORITY, B_STRING_TYPE, 0,
						&bufpos[size], strlen(&bufpos[size]) + 1);
					bufpos[len - 2] = '\r';
				}
	
				size = strsize(FIELD_MIME);
				if (!compare(bufpos, FIELD_MIME, size - 1, FALSE)) {
					bufpos[len - 2] = 0;
					file.WriteAttr(B_MAIL_ATTR_MIME, B_STRING_TYPE, 0,
						&bufpos[size], strlen(&bufpos[size]) + 1);
					bufpos[len - 2] = '\r';
				}
			}
			offset += len;
		}
	}
	return result;
}
