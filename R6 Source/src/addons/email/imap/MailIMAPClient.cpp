#include <stdio.h>
#include <stdlib.h>
#include <bsd_mem.h>

#include <malloc.h>
#include <memory.h>
#include <OS.h>
#include <String.h>
#include <File.h>
#include <DataIO.h>
#include <Message.h>

#include "MDE-mail.h"
#include "MDMailAddonDefs.h"
#include "MDMailAddon.h"
#include "MDMailHeader.h"
#include "MDMailContainer.h"
#include "MDMailHeadersList.h"
#include "MDMailboxesList.h"
#include "MDMailbox.h"
#include "MDMailList.h"
#include "MDMail.h"
#include "MDMailHeaderEntry.h"
#include "MDUtils.h"
#include "md5.h"

#include "MailIMAPTools.h"
#include "MailClientSocket.h"
#include "MailIMAPClientData.h"
#include "MailIMAPAnswerList.h"
#include "MailIMAPClient.h"

MDMailAddon* instantiate_mail_daemon_addon(image_id id)
{
	MDMailAddon* addon = new MDMailAddon(id);
  	return addon;
}

#pragma mark -

MDMailAddon::MDMailAddon(image_id id)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::MailIMAPClient\n");
	protocol_data = new MailIMAPClientData();
	fServer = NULL;
	fServerPort = 143;
	fLogin = NULL;
	fPassword = NULL;
	connected = false;
}

MDMailAddon::~MDMailAddon()
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::~MailIMAPClient\n");
	if (protocol_data)
		delete protocol_data;
	if (fServer)
		delete[] fServer;
	if (fLogin)
		delete[] fLogin;
	if (fPassword)
		delete[] fPassword;
	connected = false;
}

md_mailaddon_capability MDMailAddon::GetCapabilities()
{
	md_mailaddon_capability capabilities = (md_mailaddon_capability)(md_mailaddon_capability_login_logout | md_mailaddon_capability_mailbox_listing | \
	md_mailaddon_capability_mailbox_creation | md_mailaddon_capability_mailbox_deletion | \
	md_mailaddon_capability_mailbox_renaming | md_mailaddon_capability_text_headers_retrieving | \
	md_mailaddon_capability_headers_retrieving | md_mailaddon_capability_structure_retrieving | \
	md_mailaddon_capability_content_retrieving | md_mailaddon_capability_mailbox_cleaning);
	
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->capabilities & imap_capabilities_quota))
		capabilities = (md_mailaddon_capability)(capabilities | md_mailaddon_capability_server_space);
	return capabilities;
}

uint32 MDMailAddon::GetDirection()
{
	return (md_mailaddon_direction_incoming | md_mailaddon_direction_outgoing);
}

#pragma mark -

status_t MDMailAddon::Connect(const char *server, uint16 port, bool secured)
{
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if (data)
	{
		// initialize the different variables
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Connect : -%s- %d %d\n",server,port,secured);
		if (server)
		{
			fServer = (char *)malloc(strlen(server)+1);
			strcpy(fServer,server);
		}
		fServerPort = port;
	
		// lets connect !!!
		data->state = imap_state_connecting;
		error = data->socket.Connect(fServer,fServerPort,secured);
		if (error == B_NO_ERROR)
		{
			MAIL_CLIENT_IMAP_PRINT ("imap Connected\n");
			connected = true;
			
			// connected, so waiting for the greeting string
			BString my_string;
			if (!data->socket.RecvString(&my_string))
			{
				MAIL_CLIENT_IMAP_PRINT ("error receiving welcome string\n");
				return B_ERROR;
			}
			MAIL_CLIENT_IMAP_PRINT ("welcome imap : -%s-\n",my_string.String());
			data->state = imap_state_capability;
			error = data->capability();
		}
		else
			printf("MailIMAPClient::Connect : error connect : %lx\n",error);
	}
	return error;
}

bool MDMailAddon::IsConnected()
{
	return connected;
}

status_t MDMailAddon::Login(const char *login, const char *password, md_mailaddon_authentication authentication)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : -%s-/-%s-\n",login,password);
	
	// parameters
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if (data)
	{
		if (login)
		{
			fLogin = (char *)malloc(strlen(login)+1);
			memset(fLogin,0,strlen(login));
			strcpy(fLogin,login);
		}
		else
			return B_ERROR;
		if (password)
		{
			fPassword = (char *)malloc(strlen(password)+1);
			memset(fPassword,0,strlen(password));
			strcpy(fPassword,password);
		}
		else
			return B_ERROR;
			
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : authentication = %x/%x\n",authentication,data->authentication);

		// Doing authentication...
		switch (authentication)
		{
			case md_mailaddon_authentication_login:
				if (data->authentication & imap_authentication_login)
				{
					BString login_string(data->generate_tag("AUTHENTICATE LOGIN"));
					login_string+="\r\n";
					if (data->socket.SendString(login_string.String()))
					{
						BString received_string;
						if (data->socket.RecvString(&received_string))
						{
							// check if we have a '+' at the beginning...
							if (received_string.ByteAt(0) == '+')
							{
								// remove the '+'
								received_string.Remove(0,1);
								
								// remove the next spaces...
								while(received_string.ByteAt(0) == ' ')
								{
									received_string.Remove(0,1);
								}
								
								// decode the "User" string
								base64_decode_string(&received_string);
								MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : received_string (LOGIN) 1 : -%s-\n",received_string.String());

								// check if the string contains "User"
								if (received_string.IFindFirst("User") == -1)
									return B_ERROR;
									
								// Send the username
								BString sent_buffer(login);
								if (base64_encode_string(&sent_buffer) == false)
									return B_ERROR;
								
								MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : sent string (LOGIN) 2 : -%s-\n",sent_buffer.String());
								sent_buffer+="\r\n";
								if (data->socket.SendString(sent_buffer.String()))
								{
									BString received_string;
									if (data->socket.RecvString(&received_string))
									{
										// check if we have a '+' at the beginning...
										if (received_string.ByteAt(0) == '+')
										{
											// remove the '+'
											received_string.Remove(0,1);
											
											// remove the next spaces...
											while(received_string.ByteAt(0) == ' ')
											{
												received_string.Remove(0,1);
											}
											
											// decode the "User" string
											base64_decode_string(&received_string);
											MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : received_string (LOGIN) 2 : -%s-\n",received_string.String());
			
											// check if the string contains "Pass"
											if (received_string.IFindFirst("Pass") == -1)
												return B_ERROR;
												
											// Send the password
											BString sent_buffer(password);
											if (base64_encode_string(&sent_buffer) == false)
												return B_ERROR;
											
											MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : sent string (LOGIN) 2 : -%s-\n",sent_buffer.String());
											if (data->socket.SendString(sent_buffer.String()) == 0)
											{
												printf("MailIMAPClient::Login : error AUTHENTICATE LOGIN sending password\n");
												return B_ERROR;
											}

											// Receive the final answer
											MailIMAPAnswerList answer_list;
											bool server_answer = false;
											if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
											{
												#ifdef MAIL_CLIENT_IMAP_DEBUG
												answer_list.PrintToStream();
												#endif
												
												if (server_answer)
												{
													data->state = imap_state_transaction;
													error = B_NO_ERROR;
												}
											}
											else
												printf("MDMailAddon::Login : LOGIN error\n");

											data->state = imap_state_transaction;
											error = B_NO_ERROR;
										}
									}
								}
							}
						}
					}
					else
						printf("MDMailAddon::Login : error sending AUTHENTICATE LOGIN\n");
				}
				break;
				
			case md_mailaddon_authentication_plain:
//				if (data->authentication & imap_authentication_plain)
				{
					BString login_string(data->generate_tag("LOGIN "));
					login_string+=login;
					login_string+=" ";
					login_string+=password;
					login_string+="\r\n";
					if (data->socket.SendString(login_string.String()))
					{
						MailIMAPAnswerList answer_list;
						bool server_answer = false;
						if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
						{
							#ifdef MAIL_CLIENT_IMAP_DEBUG
							answer_list.PrintToStream();
							#endif
							
							if (server_answer)
							{
								data->state = imap_state_transaction;
								error = B_NO_ERROR;
							}
						}
						else
							error = B_ERROR;
					}
					else
						error = B_ERROR;
				}
				break;

			case md_mailaddon_authentication_cram_md5:
				if (data->authentication & imap_authentication_cram_md5)
				{
					BString login_string(data->generate_tag("AUTHENTICATE CRAM-MD5"));
					login_string+="\r\n";
					if (data->socket.SendString(login_string.String()))
					{
						BString received_string;
						if (data->socket.RecvString(&received_string))
						{
							// check if we have a '+' at the beginning...
							if (received_string.ByteAt(0) == '+')
							{
								// remove the '+'
								received_string.Remove(0,1);
								
								// remove the next spaces...
								while(received_string.ByteAt(0) == ' ')
								{
									received_string.Remove(0,1);
								}
								MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : received_string (CRAM-MD5) 0 : -%s-\n",received_string.String());
								
								// decode the "Tag" string
								base64_decode_string(&received_string);
								MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : received_string (CRAM-MD5) 1 : -%s-\n",received_string.String());
					
								unsigned char digest[17]="";
								int32 text_len = received_string.Length();
								unsigned char *text = (unsigned char *)received_string.String();
							
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
			
								MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : sent string (CRAM-MD5) 1 : -%s-\n",ch_hshbuf.String());
								BString hshbuf64(ch_hshbuf.String());
								base64_encode_string(&hshbuf64);
								MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Login : sent string (CRAM-MD5) 2 : -%s-\n",hshbuf64.String());
								hshbuf64+="\r\n";
											
								if (data->socket.SendString(hshbuf64.String()) == 0)
								{
									printf("MailIMAPClient::Login : error AUTHENTICATE CRAM-MD5 sending tag\n");
									return B_ERROR;
								}

								// Receive the final answer
								MailIMAPAnswerList answer_list;
								bool server_answer = false;
								if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
								{
									#ifdef MAIL_CLIENT_IMAP_DEBUG
									answer_list.PrintToStream();
									#endif
									if (server_answer)
									{
										data->state = imap_state_transaction;
										error = B_NO_ERROR;
									}
								}
								else
									printf("MDMailAddon::Login : CRAM-MD5 error\n");
							}
						}
					}

				}
				break;

			case md_mailaddon_authentication_digest_md5:
				if (data->authentication & imap_authentication_digest_md5)
				{
				}
				error = B_ERROR;
				break;

			default:
				error = B_ERROR;
				break;
		}
	}
	return error;
}

status_t MDMailAddon::Logout()
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::Logout\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if (data)
	{
		if (data->socket.SendString(data->generate_tag("LOGOUT\r\n")))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif
				if (server_answer)
					error = B_NO_ERROR;
			}
			else
				printf("MDMailAddon::Logout : error\n");
		}
	}
	data->socket.Close();
	data->selected_mailbox = "";
	data->predicted_uid = "";
	connected = false;
	data->state = imap_state_nothing;
	return error;
}

status_t MDMailAddon::GetServerAvailableSpace(const char *mailbox, off_t *storage_current, off_t *storage_limit, int32 *message_current, int32 *message_limit)
{
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((mailbox) && (storage_current) && (storage_limit) && (message_current) && (message_limit) && (data))
	{
		if (data->capabilities & imap_capabilities_quota)
		{
			BString full_path(data->generate_tag("GETQUOTAROOT "));
			full_path+=mailbox;
			full_path+="\r\n";
			if (data->socket.SendString(full_path.String()))
			{
				MailIMAPAnswerList answer_list;
				bool server_answer = false;
				if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
				{
					#ifdef MAIL_CLIENT_IMAP_DEBUG
					answer_list.PrintToStream();
					#endif
					
					if (server_answer)
					{
						for (int32 i=0;i<answer_list.CountItems();i++)
						{
							BString *ch = (BString *)answer_list.ItemAt(i);
							if ((ch) && (ch->ByteAt(0) == '*'))
							{
								// QUOTAROOT result
								if (ch->FindFirst("QUOTAROOT") >= 0)
								{
									continue;
								}
								
								// QUOTA result
								if (ch->FindFirst("QUOTA") >= 0)
								{
									BString string2;
									balance_string(ch->String(),&string2,'(',')');
									int32 index_start = string2.FindFirst('(');
									if (index_start >= 0)
									{
										string2.Remove(0,index_start+1);
										
										// cut end
										int32 index_end = string2.FindFirst(')');
										if (index_start >= 0)
										{
											string2.Remove(index_end,string2.Length()-index_end);
											
											if (string2.FindFirst("STORAGE") >= 0)
											{
												string2.Remove(0,strlen("STORAGE")+1);
												sscanf(string2.String(),"%Ld %Ld",storage_current, storage_limit);
											}

											if (string2.FindFirst("MESSAGE") >= 0)
											{
												string2.Remove(0,strlen("MESSAGE")+1);
												sscanf(string2.String(),"%ld %ld",message_current, message_limit);
											}
										}
									}
								}
							}
						}
						return B_NO_ERROR;
					}
				}
			}
		}
		*storage_current = -1;
		*storage_limit = -1;
		*message_current = -1;
		*message_limit = -1;
	}
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::SetMailboxesRoot(const char *)
{
	return B_NO_ERROR;
}

status_t MDMailAddon::GetMailboxStats(const char *name, uint32 *total_messages, uint32 *new_messages)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailboxStats\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (name) && (strlen(name) > 0) && (total_messages) && (new_messages))
	{
		*total_messages = 0;
		*new_messages = 0;
		BString full_path(data->generate_tag("STATUS "));
		full_path+=name;
		full_path+=" (UIDNEXT MESSAGES UNSEEN)\r\n";
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailboxStats : -%s-\n",full_path.String());
		if (data->socket.SendString(full_path.String()))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif
				
				if (server_answer)
				{
					// parsing the answer...
					for (int32 i=0;i<answer_list.CountItems();i++)
					{
						BString *pt = (BString *)answer_list.ItemAt(i);
						if (pt)
						{
							int32 index_messages = pt->FindFirst("MESSAGES");
							int32 index_unseen = pt->FindFirst("UNSEEN");
							int32 index_uidnext = pt->FindFirst("UIDNEXT");
							if ((index_messages >= 0) && (index_unseen >= 0) && (index_uidnext >= 0))
							{
								// parsing MESSAGES parameter
								BString pt2 = *pt;
								pt2.Remove(0,index_messages+strlen("MESSAGES")+1);
								
								int32 index_space = pt2.FindFirst(" ");
								if (index_space >= 0)
								{
									pt2.Remove(index_space,pt2.Length()-index_space);
									*total_messages = atoi(pt2.String());
									MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailboxStats : total_messages = %ld\n",*total_messages);
								}
								
								// parsing UNSEEN parameter
								pt2 = *pt;
								pt2.Remove(0,index_unseen+strlen("UNSEEN")+1);
								
								index_space = pt2.FindFirst(" ");
								if (index_space >= 0)
								{
									pt2.Remove(index_space,pt2.Length()-index_space);
									*new_messages = atoi(pt2.String());
									MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailboxStats : new_messages = %ld\n",*new_messages);
								}
								
								// parsing UIDNEXT parameter
								pt2 = *pt;
								pt2.Remove(0,index_uidnext+strlen("UIDNEXT")+1);
								index_space = pt2.FindFirst(")");
								if (index_space >= 0)
								{
									pt2.Remove(index_space,pt2.Length()-index_space);
									data->predicted_uid = pt2.String();
									MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailboxStats : predicted_uid = -%s-\n",data->predicted_uid.String());
								}
								error = B_NO_ERROR;
							}
						}
					}
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::SelectMailbox(const char *name)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::SelectMailbox\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (name) && (strlen(name) > 0))
	{
		if (data->selected_mailbox != name)
		{
			// Deselect the previous mailbox first
			DeselectMailbox();
			
			BString full_path(data->generate_tag("SELECT "));
			full_path+=name;
			full_path+="\r\n";
			if (data->socket.SendString(full_path.String()))
			{
				MailIMAPAnswerList answer_list;
				bool server_answer = false;
				if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
				{
					#ifdef MAIL_CLIENT_IMAP_DEBUG
					answer_list.PrintToStream();
					#endif
					
					if (server_answer)
					{
						data->selected_mailbox = name;
						data->analyse_select(&answer_list);

						// Get the latest info on the mailbox						
						uint32 total_messages = 0;
						uint32 new_messages = 0;
						GetMailboxStats(name, &total_messages, &new_messages);
						error = B_NO_ERROR;
					}
				}
			}
		}
		else
		{
			uint32 total_messages = 0;
			uint32 new_messages = 0;
			error = GetMailboxStats(name, &total_messages, &new_messages);
		}
	}
	return error;
}

status_t MDMailAddon::SelectMailboxRO(const char *name)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::SelectMailboxRO\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (name) && (strlen(name) > 0))
	{
		if (data->selected_mailbox != name)
		{
			// Deselect the previous mailbox first
			DeselectMailbox();
				
			BString full_path(data->generate_tag("EXAMINE "));
			full_path+=name;
			full_path+="\r\n";
			if (data->socket.SendString(full_path.String()))
			{
				MailIMAPAnswerList answer_list;
				bool server_answer = false;
				if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
				{
					#ifdef MAIL_CLIENT_IMAP_DEBUG
					answer_list.PrintToStream();
					#endif
					
					if (server_answer)
					{
						data->selected_mailbox = name;
						data->analyse_select(&answer_list);
						
						// Get the latest info on the mailbox						
						uint32 total_messages = 0;
						uint32 new_messages = 0;
						GetMailboxStats(name, &total_messages, &new_messages);
						error = B_NO_ERROR;
					}
				}
			}
		}
		else
		{
			uint32 total_messages = 0;
			uint32 new_messages = 0;
			error = GetMailboxStats(name, &total_messages, &new_messages);
		}
	}
	return error;
}

status_t MDMailAddon::DeselectMailbox()
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::DeselectMailbox\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction))
	{
		if (data->socket.SendString(data->generate_tag("CLOSE\r\n")))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif
				
				if (server_answer)
				{
					data->selected_mailbox = "";
					data->predicted_uid = "";
					error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::CleanupMailbox()
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::CleanupMailbox\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (data->selected_mailbox != "") && (data->socket.SendString(data->generate_tag("EXPUNGE\r\n"))))
	{
		MailIMAPAnswerList answer_list;
		bool server_answer = false;
		if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
		{
			#ifdef MAIL_CLIENT_IMAP_DEBUG
			answer_list.PrintToStream();
			#endif
			
			if (server_answer)
			{
				error = B_NO_ERROR;
			}
		}
	}
	return error;
}

#pragma mark -

status_t MDMailAddon::GetMailboxesList(MDMailboxesList *list, bool get_mailbox_stats)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailboxesList\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (data->socket.SendString(data->generate_tag("LIST \"\" \"*\"\r\n"))))
	{
		MailIMAPAnswerList answer_list;
		bool server_answer = false;
		if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
		{
			if (server_answer)
			{
				for (int32 i=0;i<answer_list.CountItems();i++)
				{
					BString *ch = (BString *)answer_list.ItemAt(i);
					if ((ch) && (ch->ByteAt(0) == '*'))
					{
						MDMailbox *mailbox = list->AddFolderFromIMAP(ch->String());
						if (get_mailbox_stats)
						{
							GetMailboxStats(mailbox->name.String(),&mailbox->total_messages,&mailbox->new_messages);
						}
					}
				}
				error = B_NO_ERROR;
			}
		}
	}
	return error;
}

status_t MDMailAddon::GetSubscribedMailboxesList(MDMailboxesList *list, bool get_mailbox_stats)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetSubscribedMailboxesList\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (data->socket.SendString(data->generate_tag("LSUB \"\" \"*\"\r\n"))))
	{
		MailIMAPAnswerList answer_list;
		bool server_answer = false;
		if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
		{
//			answer_list.PrintToStream();
			if (server_answer)
			{
				for (int32 i=0;i<answer_list.CountItems();i++)
				{
					BString *ch = (BString *)answer_list.ItemAt(i);
					if ((ch) && (ch->ByteAt(0) == '*'))
					{
						MDMailbox *mailbox = list->AddFolderFromIMAP(ch->String());
						if (get_mailbox_stats)
						{
							GetMailboxStats(mailbox->name.String(),&mailbox->total_messages,&mailbox->new_messages);
						}
					}
				}
				error = B_NO_ERROR;
			}
		}
	}
	return error;
}

status_t MDMailAddon::CreateMailbox(const char *ref, const char *name)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::CreateFolder\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (name) && (strlen(name) > 0))
	{
		BString full_path(data->generate_tag("CREATE "));
		if ((ref) && (strlen(ref) > 0))
		{
			full_path+=ref;
			full_path+="/";
		}
		full_path+=name;
		full_path+="\r\n";
		if (data->socket.SendString(full_path.String()))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif
				
				if (server_answer)
				{
					error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::DeleteMailbox(const char *name)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::DeleteMailbox\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (name) && (strlen(name) > 0))
	{
		BString full_path(data->generate_tag("DELETE "));
		full_path+=name;
		full_path+="\r\n";
		if (data->socket.SendString(full_path.String()))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif
				
				if (server_answer)
				{
					error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::RenameMailbox(const char *old_name, const char *new_name)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::RenameMailbox\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (old_name) && (new_name))
	{
		if ((strlen(old_name) > 0) && (strlen(new_name) > 0))
		{
			BString full_path(data->generate_tag("RENAME "));
			full_path+=old_name;
			full_path+=" ";
			full_path+=new_name;
			full_path+="\r\n";
			if (data->socket.SendString(full_path.String()))
			{
				MailIMAPAnswerList answer_list;
				bool server_answer = false;
				if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
				{
					#ifdef MAIL_CLIENT_IMAP_DEBUG
					answer_list.PrintToStream();
					#endif
					
					if (server_answer)
					{
						error = B_NO_ERROR;
					}
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::SubscribeMailbox(const char *name)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::SubscribeMailbox\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (name) && (strlen(name) > 0))
	{
		BString full_path(data->generate_tag("SUBSCRIBE "));
		full_path+=name;
		full_path+="\r\n";
		if (data->socket.SendString(full_path.String()))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif
				
				if (server_answer)
				{
					error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::UnsubscribeMailbox(const char *name)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::UnsubscribeMailbox\n");
	status_t error = B_ERROR;
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (data->state == imap_state_transaction) && (name) && (strlen(name) > 0))
	{
		BString full_path(data->generate_tag("UNSUBSCRIBE "));
		full_path+=name;
		full_path+="\r\n";
		if (data->socket.SendString(full_path.String()))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif
				
				if (server_answer)
				{
					error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

#pragma mark -

status_t MDMailAddon::GetMailHeaders(const char *uid,MDMailHeader *header, const BMessage *needed_headers)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeaders : -%s-\n",uid);
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((uid) && (data) && (header))
	{
		header->Empty();
		
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeaders : -%s-\n",uid);

		// send command to request headers UID FETCH
		BString sent_string;
		data->GetMailHeaders(&sent_string,uid,needed_headers);
		if (data->socket.SendString(data->generate_tag(sent_string.String())))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				#ifdef MAIL_CLIENT_IMAP_DEBUG
				answer_list.PrintToStream();
				#endif

				if (server_answer)
				{
					bool header_received = false;
					for (int32 i=0;i<answer_list.CountItems();i++)
					{
						BString *ch = (BString *)answer_list.ItemAt(i);
						
						if (needed_headers)
						{
							// parsing headers
							if ((ch) && (ch->ByteAt(0) != '*') && (ch->FindFirst(IMAP_TAG) != 0) && (ch->FindFirst(")") != 0))
							{
								data->parse_header(ch->String(),header);
								header_received = true;
							}
						}

						// parsing structure
						if ((ch) && (ch->ByteAt(0) == '*'))
						{
							int32 index = ch->FindFirst("FETCH");
							if (index >= 0)
							{
								ch->Remove(0,index+strlen("FETCH")+1);
								populate_header_with_imap(ch->String(),header);
								header_received = true;
							}
						}
					}
					if (header_received)
						error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::GetMailHeadersStart(const char *index_start,const char *index_end, const BMessage *needed_headers)
{
	MAIL_CLIENT_IMAP_PRINT("MDMailAddon::GetMailHeadersStart\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;

	if ((index_start) && (index_end) && (data) && (data->state == imap_state_transaction) && (data->selected_mailbox != ""))
	{
		int64 start = atoi(index_start);
		int64 end = atoi(index_end);

		// checking if the user wants everything...
		if ((start == -1) && (end == -1))
		{
			start = 1;
			end = atoi(data->predicted_uid.String())-1;
		}
		else
			if (end == -1)
				end = atoi(data->predicted_uid.String())-1;
		
		if (start == -1)
			start = 1;
			
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeadersStart : %Ld:%Ld\n",start,end);

		if ((end - start) >= 0)
		{
			MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeadersStart : %Ld:%Ld\n",start,end);
	
			// send command to request headers
			BString sent_string;
			data->GetMailHeaders(&sent_string,start,end,needed_headers);
			MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeadersStart : -%s-\n",sent_string.String());
			if (data->socket.SendString(data->generate_tag(sent_string.String())))
			{
				error = B_NO_ERROR;
			}
		}
	}
	return error;
}

status_t MDMailAddon::GetMailHeaders(MDMailHeadersList *list, bool *done, int32 nb)
{
	MAIL_CLIENT_IMAP_PRINT("MDMailAddon::GetMailHeaders\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((done) && (data) && (data->state == imap_state_transaction) && (list) && data->selected_mailbox != "")
	{
		// get the list
		list->Empty();
			
		MailIMAPAnswerList answer_list;
		bool server_answer = false;
		if (data->load_server_response(&answer_list,&server_answer,nb,')',' ') == B_NO_ERROR)
		{
//			#ifdef MAIL_CLIENT_IMAP_DEBUG
//			answer_list.PrintToStream();
//			#endif

			*done = server_answer;

			bool header_received = false;
			MDMailHeader *mailheader = NULL;
			for (int32 i=0;i<answer_list.CountItems();i++)
			{
				BString *ch = (BString *)answer_list.ItemAt(i);

				// parsing headers
				if ((ch) && (ch->ByteAt(0) != '*') && (ch->FindFirst(IMAP_TAG) != 0) && (ch->FindFirst(")") != 0))
				{
					if (mailheader)
					{
						data->parse_header(ch->String(),mailheader);
						header_received = true;
					}
					else
						printf("MailIMAPClient::GetMailHeaders : mail == NULL\n");
				}
			
				// parsing structure
				if ((ch) && (ch->ByteAt(0) == '*'))
				{
					int32 index = ch->FindFirst("FETCH");
					if (index >= 0)
					{
						ch->Remove(0,index+strlen("FETCH")+1);

						mailheader = new MDMailHeader();
						list->AddItem(mailheader);
						populate_header_with_imap(ch->String(),mailheader);
						header_received = true;
					}
				}
			}
			if ((server_answer) || (header_received))
				error = B_NO_ERROR;
		}
	}
	return error;
}

#pragma mark -

status_t MDMailAddon::GetMailStructure(const char *uid, MDMail *mail)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailStructure\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((uid) && (data) && (mail))
	{
		mail->Empty();
		
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailStructure : -%s-\n",uid);

		// send command to request headers
		char string[255];	
		sprintf (string,"UID FETCH %s BODY\r\n",uid);
		
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailStructure : -%s-",data->generate_tag(string));
		if (data->socket.SendString(data->generate_tag(string)))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
//				#ifdef MAIL_CLIENT_IMAP_DEBUG
//				answer_list.PrintToStream();
//				#endif

				if (server_answer)
				{
					bool header_received = false;
					for (int32 i=0;i<answer_list.CountItems();i++)
					{
						BString *ch = (BString *)answer_list.ItemAt(i);
						if ((ch) && (ch->ByteAt(0) == '*'))
						{
							int32 index = ch->FindFirst("FETCH");
							if (index >= 0)
							{
								ch->Remove(0,index+strlen("FETCH")+1);
								populate_structure_with_imap(ch->String(),mail,false);
								header_received = true;
							}
						}
					}
					if (header_received)
						error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::GetMailStructureStart(const char *index_start,const char *index_end)
{
	MAIL_CLIENT_IMAP_PRINT("MDMailAddon::GetMailStructureStart\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((index_start) && (index_end) && (data) && (data->state == imap_state_transaction) && (data->selected_mailbox != ""))
	{
		int64 start = atoi(index_start);
		int64 end = atoi(index_end);

		// checking if the user wants everything...
		if ((start == -1) && (end == -1))
		{
			start = 1;
			end = atoi(data->predicted_uid.String())-1;
		}
		else
			if (end == -1)
				end = atoi(data->predicted_uid.String())-1;
		
		if (start == -1)
			start = 1;
		
		if ((end - start) >= 0)
		{
			MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailStructureStart : %Ld:%Ld\n",start,end);
	
			// send command to request headers
			char string[255];	
			sprintf (string,"UID FETCH %Ld:%Ld BODY\r\n",start,end);
			if (data->socket.SendString(data->generate_tag(string)))
			{
				error = B_NO_ERROR;
			}
		}
	}
	return error;
}
	
status_t MDMailAddon::GetMailStructure(MDMailList *list,bool *done, int32 nb)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailStructure\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((data) && (list) && (done))
	{
		list->Empty();
		
		MailIMAPAnswerList answer_list;
		bool server_answer = false;
		if (data->load_server_response(&answer_list,&server_answer,nb,'*') == B_NO_ERROR)
		{
			answer_list.SortHeadersStructure();

//			#ifdef MAIL_CLIENT_IMAP_DEBUG
//			answer_list.PrintToStream();
//			#endif

			*done = server_answer;
			
			bool header_received = false;
			for (int32 i=0;i<answer_list.CountItems();i++)
			{
				BString *ch = (BString *)answer_list.ItemAt(i);
				if ((ch) && (ch->ByteAt(0) == '*'))
				{
					int32 index = ch->FindFirst("FETCH");
					if (index >= 0)
					{
						ch->Remove(0,index+strlen("FETCH")+1);

						MDMail *mail = new MDMail();
						populate_structure_with_imap(ch->String(),mail,true);
						list->AddItem(mail);
						header_received = true;
					}
				}
			}
			if ((server_answer) || (header_received))
				error = B_NO_ERROR;
		}
	}
	return error;
}

#pragma mark -

status_t MDMailAddon::GetMailHeadersStructure(const char *uid, MDMail *mail,const BMessage *needed_headers)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeadersStructure : -%s-\n",uid);
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((uid) && (data) && (mail))
	{
		mail->Empty();
		
		MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeadersStructure : -%s-\n",uid);

		// send command to request headers UID FETCH
		BString sent_string;
		data->GetMailHeadersStructure(&sent_string,uid,needed_headers);
		if (data->socket.SendString(data->generate_tag(sent_string.String())))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
				answer_list.SortHeadersStructure();

//				#ifdef MAIL_CLIENT_IMAP_DEBUG
//				answer_list.PrintToStream();
//				#endif

				if (server_answer)
				{
					bool header_received = false;
					for (int32 i=0;i<answer_list.CountItems();i++)
					{
						BString *ch = (BString *)answer_list.ItemAt(i);
						if (ch)
						{
							// parsing headers
							if ((ch->ByteAt(0) != '*') && (ch->FindFirst(IMAP_TAG) != 0) && (ch->FindFirst(")") != 0) && (i >= answer_list.cnt_answer))
							{
								data->parse_header(ch->String(),&mail->header);
								header_received = true;
							}
							else
							{
								// parsing structure
								if ((ch->ByteAt(0) == '*') || (i < answer_list.cnt_answer))
								{
									// analyse FETCH answer
									int32 index = ch->FindFirst("FETCH");
									if (index >= 0)
										ch->Remove(0,index+strlen("FETCH")+1);
									populate_structure_with_imap(ch->String(),mail,true);
									populate_header_with_imap(ch->String(),&mail->header);
									header_received = true;
								}
							}
						}
					}
					if (header_received)
						error = B_NO_ERROR;
				}
			}
		}
	}
	return error;
}

status_t MDMailAddon::GetMailHeadersStructureStart(const char *index_start, const char *index_end, const BMessage *needed_headers)
{
	MAIL_CLIENT_IMAP_PRINT("MDMailAddon::GetMailHeadersStructureStart\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((index_start) && (index_end) && (data) && (data->state == imap_state_transaction) && (data->selected_mailbox != ""))
	{
		int64 start = atoi(index_start);
		int64 end = atoi(index_end);

		// checking if the user wants everything...
		if ((start == -1) && (end == -1))
		{
			start = 1;
			end = atoi(data->predicted_uid.String())-1;
		}
		else
			if (end == -1)
				end = atoi(data->predicted_uid.String())-1;
		
		if (start == -1)
			start = 1;
		
		if ((end - start) >= 0)
		{
			MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailHeadersStructureStart : %Ld:%Ld\n",start,end);
	
			// send command to request headers
			BString sent_string;
			data->GetMailHeadersStructure(&sent_string,start,end,needed_headers);
			if (data->socket.SendString(data->generate_tag(sent_string.String())))
			{
				error = B_NO_ERROR;
			}
		}
	}
	return error;
}

status_t MDMailAddon::GetMailHeadersStructure(MDMailList *list,bool *done, int32 nb)
{
	MAIL_CLIENT_IMAP_PRINT("MDMailAddon::GetMailHeadersStructure\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	status_t error = B_ERROR;
	if ((done) && (data) && (data->state == imap_state_transaction) && (list) && data->selected_mailbox != "")
	{
		// get the list
		list->Empty();
			
		MailIMAPAnswerList answer_list;
		bool server_answer = false;
		if (data->load_server_response(&answer_list,&server_answer,nb,')',' ') == B_NO_ERROR)
		{
			answer_list.SortHeadersStructureMult();

//			#ifdef MAIL_CLIENT_IMAP_DEBUG
//			answer_list.PrintToStream();
//			#endif

			*done = server_answer;

			bool header_received = false;
			MDMail *mail = NULL;
			for (int32 i=0;i<answer_list.CountItems();i++)
			{
				BString *ch = (BString *)answer_list.ItemAt(i);

				// parsing headers
				if ((ch) && (ch->ByteAt(0) != '*') && (ch->FindFirst(IMAP_TAG) != 0) && (ch->FindFirst(")") != 0))
				{
					if (mail)
					{
						data->parse_header(ch->String(),&mail->header);
						header_received = true;
					}
					else
						printf("MailIMAPClient::GetMailHeadersStructure : mail == NULL\n");
				}
						
				// parsing structure
				bool add = false;
				if ((ch) && (ch->ByteAt(0) == '*'))
				{
					mail = new MDMail();
					add = true;
				}
				
				int32 index = ch->FindFirst("FETCH");
				if (index >= 0)
					ch->Remove(0,index+strlen("FETCH")+1);

				populate_structure_with_imap(ch->String(),mail,true);
				populate_header_with_imap(ch->String(),&mail->header);
				header_received = true;
					
				if (add)
				{
					list->AddItem(mail);
				}
			}
			if ((server_answer) || (header_received))
				error = B_NO_ERROR;
		}
	}
	return error;
}

#pragma mark -

status_t MDMailAddon::SendData(MDMail *md_mail, const char *mailbox)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::SendData\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((md_mail) && (data) && (mailbox))
	{
		MDMailMessage mail;
		MDMailHeaderEntry *entry = NULL;
		int32 cnt = 0;
		
		mail.AddHeaderField("X-Mailer: ","BeIA Mail");
		
		if ((entry = md_mail->header.FindEntry("Subject")))
			mail.AddHeaderField(MD_MAIL_SUBJECT,entry->value.String());
		
		if ((entry = md_mail->header.FindEntry("From")))
			mail.AddHeaderField(MD_MAIL_FROM,entry->value.String());

		if ((entry = md_mail->header.FindEntry("Reply-To")))
			mail.AddHeaderField(MD_MAIL_ATTR_REPLY,entry->value.String());

		cnt = md_mail->header.CountEntries("To");
		for (int32 i=0;i<cnt;i++)
		{
			entry = md_mail->header.GetEntry("To",i);
			if (entry)
				mail.AddHeaderField(MD_MAIL_TO,entry->value.String());
		}

		cnt = md_mail->header.CountEntries("Cc");
		for (int32 i=0;i<cnt;i++)
		{
			entry = md_mail->header.GetEntry("Cc",i);
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
		}

		// DATA
		BMallocIO mail_buffer;
		status_t err = mail.SendMemory(&mail_buffer);
		mail_buffer.Seek(0,SEEK_SET);
		MAIL_CLIENT_IMAP_PRINT("MDMailAddon::SendData : SendMemory = %lx\n",err);
		
		if (mail_buffer.BufferLength() > 0)
			((char *)(mail_buffer.Buffer()))[mail_buffer.BufferLength()] = 0;

		if (err == B_NO_ERROR)
		{
			// sending the command APPEND
			char string[255];
			ssize_t size = (ssize_t)min_c(64 * 1024, mail_buffer.BufferLength());
			ssize_t total = 0L;
			ssize_t read = 0L;
			char *buffer = new char[size];
			if (buffer)
			{
				sprintf (string,"APPEND \"%s\" {%ld}\r\n",mailbox,mail_buffer.BufferLength());
				MAIL_CLIENT_IMAP_PRINT("MDMailAddon::SendData : DATA : -%s-\n",string);
				if (data->socket.SendString(data->generate_tag(string)))
				{
					while (read = mail_buffer.Read(buffer, min_c(64 * 1024, size)))
					{
						total += read;
						MAIL_CLIENT_IMAP_PRINT("MDMailAddon::SendData... : %ld\n",read);
		
						// sending data
						if (data->socket.SendData(buffer,read) == 0)
						{
							printf("MDMailAddon::SendData : error DATA\n");
							delete[] buffer;
							return B_ERROR;
						}
					}
					delete[] buffer;
					
					// send termination string
					if (data->socket.SendData("\r\n",2) == 0)
					{
						printf("MDMailAddon::SendData : error DATA\n");
						return B_ERROR;
					}
					
					MailIMAPAnswerList answer_list;
					bool server_answer = false;
					if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
					{
//						#ifdef MAIL_CLIENT_IMAP_DEBUG
//						answer_list.PrintToStream();
//						#endif
						
						if (server_answer)
						{
							return B_NO_ERROR;
						}
					}
				}
			}
		}
	}
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailContentStart(const char *mail_index, const char *section)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailContentStart\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (mail_index) && (data->selected_mailbox != "") && (section))
	{
		// sending the command UID FETCH
		char string[255];
		sprintf (string,"UID FETCH %s BODY[%s]\r\n",mail_index,section);
		MAIL_CLIENT_IMAP_PRINT("MDMailAddon::GetMailContentStart : FETCH : -%s-\n",string);
		if (data->socket.SendString(data->generate_tag(string)))
		{
			bool starting_start_ok = false;
			do
			{
				BString my_string;
				if (data->socket.RecvString(&my_string,true))
				{
					MAIL_CLIENT_IMAP_PRINT("MDMailAddon::GetMailContentStart : RecvString : -%s-\n",my_string.String());
					if ((!starting_start_ok) && (my_string.ByteAt(0) == '*'))
					{
						starting_start_ok = true;
						return B_NO_ERROR;
					}
					else
						return B_ERROR;
				}
			}
			while (1);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailContent(MDMailContainer *container,BMallocIO *out_buffer, int64 buffer_size,int64 *size,bool *done)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailContent\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (container ) && (out_buffer) && (data->selected_mailbox != "") && (buffer_size >= 0))
	{
		int64 local_size = 0;
		do
		{
			BString my_string;
			if (data->socket.RecvString(&my_string,true))
			{
				// checks...
				if (my_string.FindFirst(data->imap_tag.String()) == 0L)
				{
					*done = true;
					data->already_loaded_string = my_string.String();
					out_buffer->SetSize(local_size);
					return B_NO_ERROR;
				}
				else
				{
					out_buffer->Write(my_string.String(),my_string.Length());
					*size+=(int64)(my_string.Length()+1);
					local_size+=(int64)my_string.Length();
					
					if (local_size > buffer_size)
					{
						out_buffer->SetSize(local_size);
						return B_NO_ERROR;
					}
				}
			}
			else
				return B_ERROR;
			
			if (*size >= container->size)
			{
				*done = true;
				break;
			}
		}
		while (1);
			
		return B_NO_ERROR;
	}
	return B_ERROR;

}

status_t MDMailAddon::GetMailContentEnd()
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::GetMailContentEnd\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if (data)
	{
		MailIMAPAnswerList answer_list;
		bool server_answer = false;
		if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
		{
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::DeleteMail(const char *uid)
{
	MAIL_CLIENT_IMAP_PRINT("MailIMAPClient::DeleteMail\n");
	MailIMAPClientData *data = (MailIMAPClientData *)protocol_data;
	if ((data) && (uid) && (data->selected_mailbox != ""))
	{
		// sending the command UID STORE
		char string[255];
		sprintf (string,"UID STORE %s +FLAGS (\\Deleted)\r\n",uid);
		MAIL_CLIENT_IMAP_PRINT("MDMailAddon::DeleteMail : STORE : -%s-\n",string);
		if (data->socket.SendString(data->generate_tag(string)))
		{
			MailIMAPAnswerList answer_list;
			bool server_answer = false;
			if (data->load_server_response(&answer_list,&server_answer) == B_NO_ERROR)
			{
//				#ifdef MAIL_CLIENT_IMAP_DEBUG
//				answer_list.PrintToStream();
//				#endif
				
				if (server_answer)
				{
					return B_NO_ERROR;
				}
			}
		}
	}
	return B_ERROR;
}
