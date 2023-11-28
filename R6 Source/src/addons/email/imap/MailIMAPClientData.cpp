#include <SupportDefs.h>
#include <stdio.h>
#include <stdlib.h>
#include <parsedate.h>

#include <String.h>

#include "MailClientSocket.h"
#include "MailIMAPAnswerList.h"
#include "MailIMAPClient.h"
#include "MDMailHeader.h"
#include "MDMailHeaderEntry.h"
#include "MDMailAddonDefs.h"
#include "MDE-mail.h"

#include "MailIMAPClientData.h"

MailIMAPClientData::MailIMAPClientData()
{
	authentication = imap_authentication_plain;
	capabilities = imap_capabilities_none;
	version = imap_version_none;
	state = imap_state_nothing;
	parsed_entry = NULL;
}

MailIMAPClientData::~MailIMAPClientData()
{
}

void MailIMAPClientData::PrintToStream()
{
	printf("MailIMAPClientData::PrintToStream\n");
	printf("authentication = %d\n",authentication);
	printf("capabilities = %d\n",capabilities);
	printf("version = %d\n",version);
	printf("state = %d\n",state);
}

status_t MailIMAPClientData::capability()
{
	status_t error = B_ERROR;
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::capability : %d\n",state);
	if (state == imap_state_capability)
	{
		if (socket.SendString(generate_tag("CAPABILITY\r\n")))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				// got an answer !
#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
#endif

				// analysing the answer to get the protocol version
				BString ret;
				if (answer_list.Search("BAD") >= 0)
					version = imap_version_v2;
				if (answer_list.Search("IMAP4rev1") >= 0)
					version = imap_version_v4rev1;
				else
					version = imap_version_v4;

				// analysing the answer to get the authentication protocol
				if (answer_list.Search("AUTH=LOGIN") >= 0)
				{
					authentication = (imap_authentication)(authentication | imap_authentication_login);
					MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::capability : LOGIN authentication\n");
					state = imap_state_auth;
					error = B_OK;
				}
				if (answer_list.Search("AUTH=PLAIN") >= 0)
				{
					authentication = (imap_authentication)(authentication | imap_authentication_plain);
					MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::capability : PLAIN authentication\n");
					state = imap_state_auth;
					error = B_OK;
				}
				if (answer_list.Search("AUTH=CRAM-MD5") >= 0)
				{
					authentication = (imap_authentication)(authentication | imap_authentication_cram_md5);
					MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::capability : CRAM-MD5 authentication\n");
					state = imap_state_auth;
					error = B_OK;
				}
				if (answer_list.Search("AUTH=DIGEST-MD5") >= 0)
				{
					authentication = (imap_authentication)(authentication | imap_authentication_digest_md5);
					MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::capability : DIGEST-MD5 authentication\n");
					state = imap_state_auth;
					error = B_OK;
				}
				if (answer_list.Search("QUOTA") >= 0)
				{
					capabilities = (imap_capabilities)(capabilities|imap_capabilities_quota);
					MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::capability : QUOTA feature\n");
					error = B_OK;
				}
				error = B_OK;
			}
		}
		else
			printf("MailIMAPClientData::capability : error\n");
	}
	return error;
}

status_t MailIMAPClientData::load_server_response(MailIMAPAnswerList *list, bool *server_answer)
{
	status_t error = B_ERROR;
	list->Empty();
	*server_answer = false;

	if (already_loaded_string.Length() > 0)
	{
		BString *my_string = new BString(already_loaded_string.String());
		list->AddItem(my_string);
		if (my_string->ByteAt(0) != '*')
		{
//			MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::load_server_response : -%s-\n",my_string->String());
			if (my_string->FindFirst(IMAP_TAG) >= 0)
			{
				if (my_string->FindFirst("OK") >= 0)
					*server_answer = true;
				else
					*server_answer = false;
				already_loaded_string = "";
				return B_NO_ERROR;
			}
		}
	}
	
//	MAIL_CLIENT_IMAP_PRINT ("MailIMAPClientData::load_server_response\n");
	do
	{
		BString *my_string = new BString(); 
		if (socket.RecvString(my_string)==false)
		{
			printf ("MailIMAPClientData::load_server_response : error\n");
			*server_answer = false;
			delete my_string;
			return B_ERROR;
		}
		else
		{
			list->AddItem(my_string);
			if (my_string->ByteAt(0) != '*')
			{
//				MAIL_CLIENT_IMAP_PRINT("MailIMAPClientData::load_server_response : -%s-\n",my_string->String());
				if (my_string->FindFirst(IMAP_TAG) >= 0)
				{
					if (my_string->FindFirst("OK") >= 0)
						*server_answer = true;
					else
						*server_answer = false;
					return B_NO_ERROR;
				}
			}
		}
	}
	while (1);
	return error;
}

status_t MailIMAPClientData::load_server_response(MailIMAPAnswerList *list, bool *server_answer, int32 nb, char sep, char sep2)
{
	status_t error = B_ERROR;
	list->Empty();
	*server_answer = false;
	
	int32 index = 0;
	do
	{
		BString *my_string = new BString(); 
		if (socket.RecvString(my_string)==false)
		{
			printf ("MailIMAPClientData::load_server_response : error\n");
			*server_answer = false;
			delete my_string;
			return B_ERROR;
		}
		else
		{
			list->AddItem(my_string);
			if ((my_string->ByteAt(0) == sep) || (my_string->ByteAt(0) == sep2))
			{
				index++;
				if (index == nb)
				{
					*server_answer = false;
					return B_NO_ERROR;
				}
			}
				
			if (my_string->FindFirst(IMAP_TAG) >= 0)
			{
				if (my_string->FindFirst("OK") >= 0)
					*server_answer = true;
				else
					*server_answer = false;
				return B_NO_ERROR;
			}
		}
	}
	while (1);
	return error;
}

void MailIMAPClientData::analyse_select(MailIMAPAnswerList *list)
{
	if (list)
	{
		for (int32 i=0;i<list->CountItems();i++)
		{
			BString *pt = (BString *)list->ItemAt(i);
			if (pt)
			{
				int32 index = 0;
				
				BString string(*pt);
				string.Remove(0,2);
				
				// look the number of messages
				index = string.FindFirst("EXISTS");
				if (index >= 0)
				{
				}
				
				// look the number of new messages
				index = string.FindFirst("RECENT");
				if (index >= 0)
				{
				}
				
				index = string.FindFirst("FLAGS");
				if (index >= 0)
				{
				}
				
				index = string.FindFirst("UIDNEXT");
				if (index >= 0)
				{
					string.Remove(0,index+strlen("UIDNEXT")+1);
					index = string.FindFirst("]");
					if (index >= 0)
						string.Remove(index,string.Length()-index);
					predicted_uid = string.String();
				}
			}
		}
	}
}

void MailIMAPClientData::parse_header(const char *string,MDMailHeader *header)
{
	if ((string) && (header))
	{
		BString buffer(string);
		if (buffer.Length() > 0)
		{
			if ((buffer.ByteAt(0L) == ' ') || (buffer.ByteAt(0L) == '\t'))
			{
				if (parsed_entry)
				{
					buffer.Remove(0,1);
					parsed_entry->value+=buffer.String();
				}
			}
			else
			{
				int32 index = buffer.FindFirst(":");
				if (index > 0)
				{
					BString fieldName(buffer.String(),index);
					buffer.Remove(0,index+2);
					
					// check headers
					if (fieldName == "Received")
					{
						parsed_entry = NULL;
						return;
					}
	
					if (fieldName == "Delivered-To")
					{
						parsed_entry = NULL;
						return;
					}
						
					if (fieldName == "Return-Path")
					{
						parsed_entry = NULL;
						return;
					}
	
					char *decoded_buffer = rfc2047_to_utf8(buffer.String());
					parsed_entry = header->AddEntry(fieldName.String(),decoded_buffer);
					free(decoded_buffer);
					
					// parsing date
					if (fieldName == "Date")
						header->date = parsedate(buffer.String(), time((time_t *)NULL));
				}
			}
		}
		else
			parsed_entry = NULL;
	}
}

const char *MailIMAPClientData::generate_tag(const char *command)
{
	char name[48];
	bigtime_t t = real_time_clock_usecs ();
	sprintf (name,IMAP_TAG"_%08ld%8ld ",(uint32)(t>>32),(uint32)(t));
	imap_tag = name;

	returned_imap_tag = imap_tag.String();
	returned_imap_tag+=command;
	return returned_imap_tag.String();
}

void MailIMAPClientData::GetMailHeaders(BString *out_string,const char *uid, const BMessage *needed_headers)
{
	if ((out_string) && (uid))
	{
		char string[255];
		sprintf (string,"UID FETCH %s (BODY FLAGS RFC822.SIZE ",uid);
		*out_string = string;
		AddHeaders(out_string,needed_headers);
		*out_string+=")\r\n";
	}
}

void MailIMAPClientData::GetMailHeaders(BString *out_string,int64 uid_start,int64 uid_end, const BMessage *needed_headers)
{
	if (out_string)
	{
		char string[255];
		sprintf (string,"UID FETCH %Ld:%Ld (BODY FLAGS RFC822.SIZE ",uid_start,uid_end);
		*out_string = string;
		AddHeaders(out_string,needed_headers);
		*out_string+=")\r\n";
	}
}

void MailIMAPClientData::GetMailHeadersStructure(BString *out_string,const char *uid, const BMessage *needed_headers)
{
	if ((out_string) && (uid))
	{
		char string[255];
		sprintf (string,"UID FETCH %s (BODY FLAGS RFC822.SIZE ",uid);
		*out_string = string;
		AddHeaders(out_string,needed_headers);
		*out_string+=")\r\n";
	}
}

void MailIMAPClientData::GetMailHeadersStructure(BString *out_string,int64 uid_start,int64 uid_end, const BMessage *needed_headers)
{
	if (out_string)
	{
		char string[255];
		sprintf (string,"UID FETCH %Ld:%Ld (BODY FLAGS RFC822.SIZE ",uid_start,uid_end);
		*out_string = string;
		AddHeaders(out_string,needed_headers);
		*out_string+=")\r\n";
	}
}

void MailIMAPClientData::AddHeaders(BString *out_string,const BMessage *message)
{
	if (out_string)
	{
		if (message)
		{
			bool add = false;
			if (message->HasString(MAIL_CLIENT_HEADERS))
			{
				add = true;
				*out_string+="BODY[HEADER.FIELDS (";
				int32 index = 0;
				BString tmp_string;
				while((message->FindString(MAIL_CLIENT_HEADERS,index++,&tmp_string) == B_NO_ERROR))
				{
					*out_string+=tmp_string.String();
					*out_string+=" ";
				}
				if (out_string->ByteAt(out_string->Length()-1) == ' ')
					out_string->Remove(out_string->Length()-1,1);
				*out_string+=")]";
			}
				if (add)
				{
					if (out_string->ByteAt(out_string->Length()-1) == ' ')
						out_string->Remove(out_string->Length()-1,1);
				}
				else
				{
					*out_string+="BODY[HEADER.FIELDS (";
					*out_string+="SUBJECT";
					*out_string+=" FROM";
					*out_string+=" REPLY-TO";
					*out_string+=" TO";
					*out_string+=" CC";
					*out_string+=" DATE";
					*out_string+=")]";
				}
		}
		else
		{
			*out_string+="BODY[HEADER.FIELDS (";
			*out_string+="SUBJECT";
			*out_string+=" FROM";
			*out_string+=" REPLY-TO";
			*out_string+=" TO";
			*out_string+=" CC";
			*out_string+=" DATE";
			*out_string+=")]";
		}
	}
}
