#include <stdio.h>
#include <stdlib.h>

#include <malloc.h>
#include <memory.h>
#include <OS.h>
#include <String.h>
#include <File.h>
#include <bsd_mem.h>

#include "MDE-mail.h"
#include "MDMailAddonDefs.h"
#include "MailClientSocket.h"
#include "MDMailAddon.h"
#include "MDMailList.h"
#include "MDMail.h"
#include "MDMailContainer.h"
#include "MDMailHeaderEntry.h"
#include "MDUtils.h"

#include "md5.h"

#include "MDMailboxesList.h"

#include "MailSMTPClientData.h"
#include "MailSMTPDestList.h"
#include "MailSMTPAnswerList.h"
#include "MailSMTPTools.h"
#include "MailSMTPClient.h"

MDMailAddon* instantiate_mail_daemon_addon(image_id id)
{
	MDMailAddon* addon = new MDMailAddon(id);
  	return addon;
}

#pragma mark -

MDMailAddon::MDMailAddon(image_id id)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::MailSMTPClient\n");
	protocol_data = new MailSMTPClientData();
	fServer = NULL;
	fServerPort = 25;
	fLogin = NULL;
	fPassword = NULL;
	connected = false;
}

MDMailAddon::~MDMailAddon()
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::~MailSMTPClient\n");
	if (protocol_data)
		delete protocol_data;
	if (fServer)
		delete[] fServer;
	if (fLogin)
		delete[] fLogin;
	if (fPassword)
		delete[] fPassword;
}

md_mailaddon_capability MDMailAddon::GetCapabilities()
{
	return (md_mailaddon_capability)(md_mailaddon_capability_login_logout | md_mailaddon_capability_data_sending);
}

uint32 MDMailAddon::GetDirection()
{
	return (md_mailaddon_direction_outgoing);
}

#pragma mark -

status_t MDMailAddon::Connect(const char *server, uint16 port, bool secured)
{
	status_t error = B_ERROR;
	MailSMTPClientData *data = (MailSMTPClientData *)protocol_data;
	if (data)
	{
		// initialize the different variables
		MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::Connect : -%s- %d\n",server,port);
		if (server)
		{
			fServer = (char *)malloc(strlen(server)+1);
			strcpy(fServer,server);
		}
		fServerPort = port;
	
		// lets connect !!!
		data->state = smtp_state_connecting;
		error = data->socket.Connect(fServer,fServerPort);
		if (error == B_NO_ERROR)
		{
			connected = true;
			
			// connected, so waiting for the greeting string
			BString my_string;
			if (!data->socket.RecvString(&my_string))
			{
				return B_ERROR;
			}
			MAIL_CLIENT_SMTP_PRINT ("welcome smtp : -%s-\n",my_string.String());
			data->state = smtp_state_helo;
			error = data->hello(my_string.String(),"beeurope.com");
			error = B_NO_ERROR;
		}
	}
	return error;
}

bool MDMailAddon::IsConnected()
{
	return connected;
}

status_t MDMailAddon::Login(const char *login, const char *password, md_mailaddon_authentication authentication)
{
	status_t error = B_NO_ERROR;
	MailSMTPClientData *data = (MailSMTPClientData *)protocol_data;
	if (data)
	{
		// no authentication requested, so just return OK
		if (authentication == md_mailaddon_authentication_none)
			return B_NO_ERROR;
		
		// authentication needed, so check if the server is an ESMTP server
		if ((login) && (password) && (authentication != md_mailaddon_authentication_none) && (data->version == smtp_version_esmtp))
		{
			// check if the server is able to do what the client is requesting
			if ((authentication == md_mailaddon_authentication_login) && (data->authentication & smtp_authentication_login == 0))
			{
				return B_ERROR;
			} 
			
			// check if the server is able to do what the client is requesting
			if ((authentication == md_mailaddon_authentication_cram_md5) && (data->authentication & smtp_authentication_cram_md5 == 0))
			{
				return B_ERROR;
			}
			
			// starting authentication process...
			BString auth_string("AUTH ");
			
			// authentication
			switch (authentication)
			{
				case md_mailaddon_authentication_login:
					auth_string+=("LOGIN\r\n");
					break;
					
				case md_mailaddon_authentication_cram_md5:
					auth_string+=("CRAM-MD5\r\n");
					break;

				default:
					return B_ERROR;
					break;
			}
			
			MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login : auth_string.String() = %d/%d %s\n",authentication,data->authentication,auth_string.String());
			if (data->socket.SendString(auth_string.String()) == 0)
			{
				printf("MDMailAddon::Login : error AUTH\n");
				return B_ERROR;
			}
			
			// waiting for the answer
			if (authentication == md_mailaddon_authentication_login)
			{
				BString answer_string;
				bool server_answer = false;
				if (data->load_server_response(&answer_string,&server_answer,334))
				{
					if (server_answer == false)
					{
						printf("MDMailAddon::Login : error return AUTH\n");
						return B_ERROR;
					}
					
					MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login says -%s-\n",answer_string.String());
	
					answer_string.Remove(0,4);
	
					MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login : -%s-\n",answer_string.String());
					if (base64_decode_string(&answer_string) == false)
						return B_ERROR;
	
					MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login : 2 -%s-\n",answer_string.String());
					if (answer_string.IFindFirst("User") == -1)
						return B_ERROR;
					
					// Sending username
					BString username_buffer(login);
					if (base64_encode_string(&username_buffer) == false)
						return B_ERROR;
					
					MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login : 3 -%s-\n",username_buffer.String());
					if (data->socket.SendString(username_buffer.String()) == 0)
					{
						printf("MDMailAddon::Login : error AUTH sending login\n");
						return B_ERROR;
					}
					
					if (data->load_server_response(&answer_string,&server_answer,334))
					{
						if (server_answer == false)
						{
							printf("MDMailAddon::Login : error return AUTH\n");
							return B_ERROR;
						}
	
						answer_string.Remove(0,4);
						
						MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login AUTH LOGIN says -%s-\n",answer_string.String());
						if (base64_decode_string(&answer_string) == false)
							return B_ERROR;
						MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login AUTH LOGIN says -%s-\n",answer_string.String());
						
						if (answer_string.IFindFirst("Pass") == -1)
							return B_ERROR;
						
						// Sending password
						BString password_buffer(password);
						if (base64_encode_string(&password_buffer) == false)
						{
							return B_ERROR;
						}
						
						MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login : password -%s-\n",password_buffer.String());
						if (data->socket.SendString(password_buffer.String()) == 0)
						{
							printf("MDMailAddon::Login : error AUTH sending password\n");
							return B_ERROR;
						}
					
						if (data->load_server_response(&answer_string,&server_answer,235))
						{
							if (server_answer == false)
							{
								printf("MDMailAddon::Login : error return AUTH\n");
								return B_ERROR;
							}
							
							MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login AUTH LOGIN PASSWORD says -%s-\n",answer_string.String());
	
							answer_string.Remove(0,4);
							
							MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login AUTH LOGIN says -%s-\n",answer_string.String());
							error = B_NO_ERROR;
						}
					}
				}
			}
			if (authentication == md_mailaddon_authentication_cram_md5)
			{
				BString answer_string;
				bool server_answer = false;
				if (data->load_server_response(&answer_string,&server_answer,334))
				{
					if (server_answer == false)
					{
						printf("MDMailAddon::Login : error return AUTH CRAM-MD5\n");
						return B_ERROR;
					}
					answer_string.Remove(0,4);
	
					MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login : -%s-\n",answer_string.String());
					
					if (base64_decode_string(&answer_string) == false)
						return B_ERROR;
	
					if (answer_string.ByteAt(answer_string.Length()-1) != '>')
						answer_string+=">";
					
					unsigned char digest[17]="";
					int32 text_len = answer_string.Length();
					unsigned char *text = (unsigned char *)answer_string.String();
				
					int32 key_len = strlen(password);
					unsigned char *key = (unsigned char *)password;
	
					MD5_CTX context;
					unsigned char k_ipad[65];    /* inner padding -
					                             * key XORd with ipad
					                             */
					unsigned char k_opad[65];    /* outer padding -
					                             * key XORd with opad
					                             */
					unsigned char tk[16];
					int i;
					/* if key is longer than 64 bytes reset it to key=MD5(key) */
					if (key_len > 64) 
					{
						MD5_CTX      tctx;
						
						MD5Init(&tctx);
						MD5Update(&tctx, key, key_len);
						MD5Final(tk, &tctx);
						
						key = tk;
						key_len = 16;
					}
				
					/*
						* the HMAC_MD5 transform looks like:
						*
						* MD5(K XOR opad, MD5(K XOR ipad, text))
						*
						* where K is an n byte key
						* ipad is the byte 0x36 repeated 64 times
						* opad is the byte 0x5c repeated 64 times
						* and text is the data being protected
					*/
				
					/* start out by storing key in pads */
					bzero( k_ipad, sizeof k_ipad);
					bzero( k_opad, sizeof k_opad);
					bcopy( key, k_ipad, key_len);
					bcopy( key, k_opad, key_len);
				
					/* XOR key with ipad and opad values */
					for (i=0; i<64; i++)
					{
						k_ipad[i] ^= 0x36;
						k_opad[i] ^= 0x5c;
					}
					/*
					* perform inner MD5
					*/
					MD5Init(&context);                   /* init context for 1st pass */
					MD5Update(&context, k_ipad, 64);     /* start with inner pad */
					MD5Update(&context, text, text_len); /* then text of datagram */
					MD5Final(digest, &context);          /* finish up 1st pass */
				
					/*
					* perform outer MD5
					*/
					MD5Init(&context);                   /* init context for 2nd pass */
					MD5Update(&context, k_opad, 64);     /* start with outer pad */
					MD5Update(&context, digest, 16);     /* then results of 1st hash */
					MD5Final(digest, &context);          /* finish up 2nd pass */

					/* convert to printable hex */
					BString ch_hshbuf(login);
					ch_hshbuf+=' ';
				  	char hex[] = "0123456789abcdef";
					unsigned char j=0;
					for (i = 0; i < 16; i++)
					{
						ch_hshbuf+=hex[(j = digest[i]) >> 4];
						ch_hshbuf+=hex[j & 0xf];
					}

					BString hshbuf64(ch_hshbuf.String());
					base64_encode_string(&hshbuf64);

					if (data->socket.SendString(hshbuf64.String()) == 0)
					{
						return B_ERROR;
					}
					
					BString answer_string;
					bool server_answer = false;
					if (data->load_server_response(&answer_string,&server_answer,235))
					{
						if (server_answer == false)
						{
							printf("MDMailAddon::Login : error return AUTH CRAM-MD5\n");
							return B_ERROR;
						}
						MAIL_CLIENT_SMTP_PRINT("MDMailAddon::Login AUTH CRAM-MD5 says -%s-\n",answer_string.String());
						error = B_NO_ERROR;
					}
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::Logout()
{
	status_t error = B_ERROR;
	MailSMTPClientData *data = (MailSMTPClientData *)protocol_data;
	if (data)
	{
		data->socket.SendString("QUIT\r\n");
		data->socket.Close();
		error = B_NO_ERROR;
	}
	data->state = smtp_state_nothing;
	return error;
}

status_t MDMailAddon::GetServerAvailableSpace(const char *mailbox, off_t *storage_current, off_t *storage_limit, int32 *message_current, int32 *message_limit)
{
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::SetMailboxesRoot(const char *)
{
	return B_NO_ERROR;
}

status_t MDMailAddon::GetMailboxStats(const char *name, uint32 *total_messages, uint32 *new_messages)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::GetMailboxStats\n");
	return B_ERROR;
}

status_t MDMailAddon::SelectMailbox(const char *name)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::SelectMailbox\n");
	return B_ERROR;
}

status_t MDMailAddon::SelectMailboxRO(const char *name)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::SelectMailboxRO\n");
	return B_ERROR;
}

status_t MDMailAddon::DeselectMailbox()
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::DeselectMailbox\n");
	return B_ERROR;
}

status_t MDMailAddon::CleanupMailbox()
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::CleanupMailbox\n");
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailboxesList(MDMailboxesList *list, bool get_mailbox_stats)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::GetMailboxesList\n");
	return B_ERROR;
}

status_t MDMailAddon::GetSubscribedMailboxesList(MDMailboxesList *list, bool get_mailbox_stats)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::GetSubscribedMailboxesList\n");
	return B_ERROR;
}

status_t MDMailAddon::CreateMailbox(const char *ref, const char *name)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::CreateMailbox\n");
	return B_ERROR;
}

status_t MDMailAddon::DeleteMailbox(const char *name)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::DeleteMailbox\n");
	return B_ERROR;
}

status_t MDMailAddon::RenameMailbox(const char *old_name, const char *new_name)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::RenameMailbox\n");
	return B_ERROR;
}

status_t MDMailAddon::SubscribeMailbox(const char *name)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::SubscribeMailbox\n");
	return B_ERROR;
}

status_t MDMailAddon::UnsubscribeMailbox(const char *name)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::UnsubscribeMailbox\n");
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailHeaders(const char * ,MDMailHeader *, const BMessage *needed_headers)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeadersStart(const char *, const char *, const BMessage *needed_headers)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeaders(MDMailHeadersList *, bool *, int32)
{
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailStructure(const char *, MDMail *)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailStructureStart(const char *, const char *)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailStructure(MDMailList *, bool *, int32)
{
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailHeadersStructure(const char *, MDMail *, const BMessage *)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeadersStructureStart(const char *, const char *, const BMessage *needed_headers)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeadersStructure(MDMailList *, bool *, int32)
{
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::SendData(MDMail *md_mail, const char *folder)
{
	MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::SendData\n");
	MailSMTPClientData *data = (MailSMTPClientData *)protocol_data;
	if ((md_mail) && (data))
	{
		MDMailMessage mail;
		MDMailHeaderEntry *entry = NULL;
		int32 cnt = 0;
		
		mail.AddHeaderField("X-Mailer: ","BeIA Mail");
		
		if ((entry = md_mail->header.FindEntry("Subject")))
			mail.AddHeaderField(MD_MAIL_SUBJECT,entry->value.String());
		
		if ((entry = md_mail->header.FindEntry("From")))
			mail.AddHeaderField(MD_MAIL_FROM,entry->value.String());

		cnt = md_mail->header.CountEntries("To");
		MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::SendData : cnt(To) = %ld\n",cnt);
		for (int32 i=0;i<cnt;i++)
		{
			entry = md_mail->header.GetEntry("To",i);
			entry->PrintToStream();
			if (entry)
				mail.AddHeaderField(MD_MAIL_TO,entry->value.String());
		}

		cnt = md_mail->header.CountEntries("Cc");
		MAIL_CLIENT_SMTP_PRINT("MailSMTPClient::SendData : cnt(Cc) = %ld\n",cnt);
		for (int32 i=0;i<cnt;i++)
		{
			entry = md_mail->header.GetEntry("Cc",i);
			entry->PrintToStream();
			if (entry)
				mail.AddHeaderField(MD_MAIL_CC,entry->value.String());
		}

		for (int32 i=0;i<md_mail->header.bcc.CountItems();i++)
		{
			MDMailRecipient *recipient = (MDMailRecipient *)md_mail->header.bcc.ItemAt(i);
			if (recipient)
				mail.AddHeaderField(MD_MAIL_BCC,recipient->email.String());
		}

		for (int32 i=0;i<md_mail->CountItems();i++)
		{
			MDMailContainer *container = (MDMailContainer *)md_mail->ItemAt(i);
			if ((container) && (container->data))
			{
				mail.AddContent(container->data,container->size);
			}
			else
				if ((container) && (md_mail->header.filename.Length()>0))
				{
					int64 size = 0;
					status_t err = md_mail->GetContentStart(container,&size);
					MAIL_CLIENT_SMTP_PRINT("GetContentStart : err = %lx\n",err);
					
					bool done = true;
					BMallocIO data;
					data.SetSize(0L);
					err = md_mail->GetContent(&data,container,size,&done);
					MAIL_CLIENT_SMTP_PRINT("GetContent : err = %lx\n",err);

					mail.AddContent((char *)data.Buffer(),data.BufferLength());

					err = md_mail->GetContentEnd();
					MAIL_CLIENT_SMTP_PRINT("GetContentEnd : err = %lx\n",err);
				}
		}
		
		char *name_from = mail.BuildFromRecipientsString();
		
		// MAIL FROM
		if ((name_from) && (strlen(name_from) > 0))
		{
			BString sent_string("MAIL FROM: ");
			sent_string+=name_from;
			sent_string+="\r\n";
			MAIL_CLIENT_SMTP_PRINT("MAIL FROM : -%s-",sent_string.String());
			if (data->socket.SendString(sent_string.String()) == 0)
			{
				printf("MDMailAddon::SendData : error MAIL FROM\n");
				return B_ERROR;
			}
			
			// waiting for the answer
			MailSMTPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer))
			{
				answer_list.PrintToStream();
				if (server_answer == false)
				{
					printf("MDMailAddon::SendData : error return MAIL FROM\n");
					return B_ERROR;
				}
			}
		}
						
		char *name_dest = mail.BuildRecipientsString();
		// RCPT TO
		if ((name_dest) && (strlen(name_dest) > 0))
		{
			MAIL_CLIENT_SMTP_PRINT("dest_field : -%s-\n",name_dest);
			
			MailSMTPDestList list(name_dest);

			// sort recipients list by reverse domain
			list.sort();
			
			for (int32 i=0;i<list.CountItems();i++)
			{
				MailSMTPDestListItem *item = (MailSMTPDestListItem *)list.ItemAt(i);
				if (item)
				{
					BString sent_string("RCPT TO: ");
					sent_string+=item->email.String();
					sent_string+="\r\n";
					if (data->socket.SendString(sent_string.String()) == 0)
					{
						printf("MDMailAddon::SendData : error RCPT TO\n");
						return B_ERROR;
					}
			
					// waiting for the answer
					MailSMTPAnswerList answer_list;
					bool server_answer = false;
					if (data->load_server_response(&answer_list,&server_answer))
					{
						answer_list.PrintToStream();
						if (server_answer == false)
						{
							printf("MDMailAddon::SendData : error return RCPT TO\n");
							return B_ERROR;
						}
					}
				}
			}
			free(name_dest);
		}

		// DATA
		BMallocIO mail_buffer;
		status_t err = mail.SendMemory(&mail_buffer);
		mail_buffer.Seek(0,SEEK_SET);
		MAIL_CLIENT_SMTP_PRINT("MDMailAddon::SendData : SendMemory = %lx\n",err);

		if (mail_buffer.BufferLength() > 0)
			((char *)(mail_buffer.Buffer()))[mail_buffer.BufferLength()] = 0;
			
		if (err == B_NO_ERROR)
		{
			if (data->socket.SendString("DATA\r\n") == 0)
			{
				printf("MDMailAddon::SendData : error DATA\n");
				return B_ERROR;
			}
			
			// waiting for the answer
			MailSMTPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer))
			{
				answer_list.PrintToStream();
				if (server_answer == false)
				{
					printf("MDMailAddon::SendData : error return DATA\n");
					return B_ERROR;
				}
			}
				
			ssize_t size = (ssize_t)min_c(64 * 1024, mail_buffer.BufferLength());
			ssize_t total = 0L;
			ssize_t read = 0L;
			char *buffer = new char[size];
			if (buffer)
			{
				while (read = mail_buffer.Read(buffer, min_c(64 * 1024, size)))
				{
					total += read;

					// sending data
					if (data->socket.SendData(buffer,read) == 0)
					{
						printf("MDMailAddon::SendData : error DATA\n");
						delete[] buffer;
						return B_ERROR;
					}
				}
				delete[] buffer;
				
				if (data->socket.SendString("\r\n.\r\n") == 0)
				{
					printf("MDMailAddon::SendData : error DATA\n");
					return B_ERROR;
				}
				
				// waiting for the answer
				MailSMTPAnswerList answer_list;
				bool server_answer = false;
				if (data->load_server_response(&answer_list,&server_answer))
				{
					answer_list.PrintToStream();
					if (server_answer == false)
					{
						printf("MDMailAddon::SendData : error return DATA\n");
						return B_ERROR;
					}
				}
				return B_NO_ERROR;
			}
		}
		else
			printf("MailSMTPClient::SendData : argh 1\n");
	}
	else
		printf("MailSMTPClient::SendData : argh 2\n");
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailContentStart(const char * , const char *)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailContent(MDMailContainer *,BMallocIO *, int64, int64 *,bool *)
{
	return B_ERROR;
}

status_t MDMailAddon::GetMailContentEnd()
{
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::DeleteMail(const char *uid)
{
	return B_ERROR;
}

