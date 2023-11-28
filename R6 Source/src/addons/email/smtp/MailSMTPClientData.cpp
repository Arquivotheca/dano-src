#include <SupportDefs.h>
#include <stdio.h>
#include <stdlib.h>

#include <String.h>

#include "MailClientSocket.h"
#include "MailSMTPAnswerList.h"
#include "MailSMTPClient.h"

#include "MailSMTPClientData.h"

MailSMTPClientData::MailSMTPClientData()
{
	authentication = smtp_authentication_none;
	capabilities = smtp_capabilities_none;
	version = smtp_version_none;
	state = smtp_state_nothing;
}

MailSMTPClientData::~MailSMTPClientData()
{
}

void MailSMTPClientData::PrintToStream()
{
	printf("MailSMTPClientData::PrintToStream\n");
	printf("authentication = %d\n",authentication);
	printf("capabilities = %d\n",capabilities);
	printf("version = %d\n",version);
	printf("state = %d\n",state);
}

status_t MailSMTPClientData::hello(const char *greeting_string, const char *client_address)
{
	status_t error = B_ERROR;
	printf("MailSMTPClientData::capability : %d\n",state);
	if (state == smtp_state_helo)
	{
		// checking the greeting string to check if it's an ESMTP or SMTP server
		if (strstr(greeting_string,"ESMTP"))
		{
			version = smtp_version_esmtp;
		}
		else
			if (strstr(greeting_string,"SMTP"))
			{
				version = smtp_version_smtp;
			}
			
		// do the helo/ehlo
		switch(version)
		{	
			case smtp_version_esmtp:
				return ehlo(client_address);
				break;
				
			case smtp_version_smtp:
				return helo(client_address);
				break;
				
			default:
				return B_ERROR;
		}
		
	}
	return error;
}

status_t MailSMTPClientData::helo(const char *client_address)
{
	status_t error = B_ERROR;
	printf("MailSMTPClientData::helo : %d\n",state);
	if (state == smtp_state_helo)
	{
		// send "HELO..."
		BString string("HELO ");
		string+=client_address;
		string+="r\n";
		if (socket.SendString(string.String()))
		{
			// read the answer from the server
			MailSMTPAnswerList answer_list;
			bool server_answer = false;
			if (load_server_response(&answer_list,&server_answer))
			{
				answer_list.PrintToStream();
				
				// analyse the answer (if needed)
			}
		}
	}
	return error;
}

status_t MailSMTPClientData::ehlo(const char *client_address)
{
	status_t error = B_ERROR;
	printf("MailSMTPClientData::ehlo : %d\n",state);
	if (state == smtp_state_helo)
	{
		// send "EHLO..."
		BString string("EHLO ");
		string+=client_address;
		string+="\r\n";
		if (socket.SendString(string.String()))
		{
			// read the answer from the server
			MailSMTPAnswerList answer_list;
			bool server_answer = false;
			if (load_server_response(&answer_list,&server_answer))
			{
				answer_list.PrintToStream();

				// parsing the answer...
				for (int32 i=0;i<answer_list.CountItems();i++)
				{
					BString *pt = (BString *)answer_list.ItemAt(i);
					if (pt)
					{
						// AUTH FOUND !
						if (pt->FindFirst("AUTH") >= 0)
						{
							if (pt->FindFirst("LOGIN") >= 0)
								authentication = (smtp_authentication)(authentication | smtp_authentication_login);
							if (pt->FindFirst("CRAM-MD5") >= 0)
								authentication = (smtp_authentication)(authentication | smtp_authentication_cram_md5);
						}
					}
				}
			}
		}
	}
	return error;
}

status_t MailSMTPClientData::load_server_response(BString *string, bool *server_answer,int16 answer_code)
{
	status_t error = B_ERROR;
	*server_answer = false;

	MAIL_CLIENT_SMTP_PRINT ("MailSMTPClientData::load_server_response\n");
	if (socket.RecvString(string)==false)
	{
		printf ("MailSMTPClientData::load_server_response : error\n");
		*server_answer = false;
		return false;
	}
	else
	{
		BString my_string2 = string->String();
		my_string2.Remove(3,my_string2.Length()-3);
		MAIL_CLIENT_SMTP_PRINT ("MailSMTPClientData::load_server_response : -%s-\n",my_string2.String());
		
		if (atoi(my_string2.String()) == answer_code)
			*server_answer = true;
		else
			*server_answer = false;
		
		if (string->ByteAt(3) == ' ')		// end of list
			return true;
	}
	return error;
}

status_t MailSMTPClientData::load_server_response(MailSMTPAnswerList *list, bool *server_answer)
{
	status_t error = B_ERROR;
	list->Empty();
	*server_answer = false;

	MAIL_CLIENT_SMTP_PRINT ("MailSMTPClientData::load_server_response\n");
	do
	{
		BString *my_string = new BString(); 
		if (socket.RecvString(my_string)==false)
		{
			printf ("MailSMTPClientData::load_server_response : error\n");
			*server_answer = false;
			delete my_string;
			return false;
		}
		else
		{
			list->AddItem(my_string);

			BString my_string2 = my_string->String();
			my_string2.Remove(3,my_string2.Length()-3);
			MAIL_CLIENT_SMTP_PRINT ("MailSMTPClientData::load_server_response : -%s-\n",my_string2.String());
			
			if ((atoi(my_string2.String()) == 250) || (atoi(my_string2.String()) == 354) || (atoi(my_string2.String()) == 334))
				*server_answer = true;
			else
				*server_answer = false;
				
			if (my_string->ByteAt(3) == ' ')		// end of list
				return true;
		}
	}
	while (1);
	return error;
}
