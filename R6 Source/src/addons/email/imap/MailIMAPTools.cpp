#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <parsedate.h>
#include <UTF8.h>

#include "E-mail.h"

#include <String.h>


#include "MDMailHeader.h"
#include "MDMailRecipient.h"
#include "MDMail.h"
#include "MDMailContainer.h"
#include "MDUtils.h"
#include "MDMailHeaderEntry.h"

#include "MailIMAPTools.h"
#include "MailIMAPClient.h"

extern char *rfc2047_to_utf8(const char *string);

void populate_header_with_imap(const char *buffer,MDMailHeader *header)
{
	if ((buffer) && (header))
	{
//		printf("buffer : -%s-\n",buffer);
		
		BString string(buffer);
		
		// remove the first parenthese
		if (string.ByteAt(0) == '(')
		{
			string.Remove(string.Length()-1,1);	// cut end
			string.Remove(0,1);					// cut beginning
		}
		
		// get UID
		int32 index = string.IFindFirst("UID ");
		if (index >= 0)
		{
			BString string2(string);
			string2.Remove(0,index+strlen("UID "));
			int32 index2 = string2.FindFirst(' ');
			if (index2 == -1)
				index2 = string2.FindFirst(')');
			if (index2 >= 0)
				string2.Remove(index2,string2.Length()-index2);
			
			header->uid = string2.String();
		}
		
		// get FLAGS
		index = string.IFindFirst("FLAGS (");
		if (index >= 0)
		{
			BString string2(string);
			string2.Remove(0,index+strlen("FLAGS (")+1);
			int32 index2 = string2.FindFirst(')');
			if (index2 >= 0)
			{
				string2.Remove(index2,string2.Length()-index2);
				if (string2.Length() > 0)
				{
					if (string2.IFindFirst("Seen") >= 0)
						header->status = (md_mail_status)(header->status | md_mail_status_read);
					if (string2.IFindFirst("Recent") >= 0)
						header->status = (md_mail_status)(header->status | md_mail_status_new);
					if (string2.IFindFirst("New") >= 0)
						header->status = (md_mail_status)(header->status | md_mail_status_new);
					if (string2.IFindFirst("Answered") >= 0)
						header->status = (md_mail_status)(header->status | md_mail_status_replied);
					if (string2.IFindFirst("Deleted") >= 0)
						header->status = (md_mail_status)(header->status | md_mail_status_deleted);
					if (string2.IFindFirst("Flagged") >= 0)
						header->status = (md_mail_status)(header->status | md_mail_status_flagged);
//					MAIL_CLIENT_IMAP_PRINT("header->status : -%s- %x\n",string2.String(),header->status);
				}
			}
		}
		
		// Get size of the message
		index = string.IFindFirst("RFC822.SIZE");
		if (index >= 0)
		{
			BString string2(string);
			string2.Remove(0,index+strlen("RFC822.SIZE")+1);
			int32 index2 = string2.FindFirst(' ');
			if (index2 >= 0)
				string2.Remove(index2,string2.Length()-index2);
			if (string2.Length() > 0)
			{
				header->size = atoi(string2.String());
//				MAIL_CLIENT_IMAP_PRINT("header->size : %Ld\n",header->size);
			}
		}
		
		// Get date of the message
		index = string.IFindFirst("INTERNALDATE \"");
		if (index >= 0)
		{
			BString string2(string);
			string2.Remove(0,index+strlen("INTERNALDATE \"")+1);
			int32 index2 = string2.FindFirst('"');
			if (index2 >= 0)
				string2.Remove(index2,string2.Length()-index2);
			if (string2.Length() > 0)
			{
				header->date = parsedate(string2.String(), time((time_t *)NULL));
				if (header->date == -1)
					header->date = 0;
//				MAIL_CLIENT_IMAP_PRINT("header->date : %ld\n",header->date);
			}
		}
		
		// Get subject of the message
		index = string.IFindFirst("ENVELOPE (");
		if (index >= 0)
		{
			BString string_out;
			string.Remove(0,index+strlen("ENVELOPE "));
			
			// keep the interesting part
			balance_string(string.String(),&string_out,'(',')');
			
			// remove date
			index = string_out.FindFirst('"');
			if (index >= 0)
			{
				int32 index2 = string_out.FindFirst('"',index+1);
				if (index2 >= 0)
					string_out.Remove(0,index2+2);
			}
					
			// get subject
			index = string_out.FindFirst('"');
			if (index == 0)
			{
				BString string3(string_out.String());
				cut_string(&string3,'"');

				char *pt = rfc2047_to_utf8(string3.String());
				header->AddEntry("Subject",pt);
				free(pt);
				
				// remove the subject
				string_out.Remove(0,string3.Length()+3);
			}
			else
			{
				if (string_out.IFindFirst("NIL") == 0)
					string_out.Remove(0,strlen("NIL")+1);
			}
			
			// get recipients
			BString string3;
			extract_parameter(&string_out,&string3,0);			// from
			if (string3.Length() > 0)
			{
				extract_recipient_entry(&string3,header,"From");
			}
	
			extract_parameter(&string_out,&string3,2);			// replyto
			if (string3.Length() > 0)
			{
				extract_recipient_entry(&string3,header,"Reply-To");
			}
	
			extract_parameter(&string_out,&string3,3);			// to
			if (string3.Length() > 0)
			{
				extract_recipients_list_entry(&string3,header,"To");
			}
	
			extract_parameter(&string_out,&string3,4);			// cc
			if (string3.Length() > 0)
			{
				extract_recipients_list_entry(&string3,header,"Cc");
			}
	
			extract_parameter(&string_out,&string3,5);			// bcc
			extract_recipients_list(&string3,&header->bcc);
		}
	}
}

void populate_structure_with_imap(const char *buffer,MDMail *mail, bool multiple_parameters)
{
	if ((buffer) && (mail))
	{
//		printf("buffer : -%s-\n",buffer);
		
		BString string(buffer);
		
		// get BODY
		int32 index = string.IFindFirst("BODY (");
		if (index >= 0)
		{
			string.Remove(0,index+strlen("BODY "));
			string.Remove(string.Length()-1,1);	// cut end
	
			// count parentheses...
			int32 cnt_par = 0;
			while(string.ByteAt(cnt_par) == '(')
			{
				cnt_par++;
			}
			
			switch(cnt_par)
			{
				case 1:					// one part
					extract_mono_block_data(&string,mail,1,0);
					break;
				case 2:					// 2 parts (MIXED or ALTERNATIVE)
					extract_dual_block_data(&string,mail);
					break;
				case 3:					// 3 parts (MIXED and ALTERNATIVE)
					extract_triple_block_data(&string,mail, multiple_parameters);
					break;
			}
		}
	}
}

#pragma mark -

void balance_string(const char *string,BString *out,char character_start,char character_end)
{
	if (string)
	{
//		printf("balance_string : -%s-\n",string->String());
		out->SetTo(string,strlen(string));
		int32 cpt_balance = 0;
		int32 index_start = out->FindFirst(character_start);
		if (index_start >= 0)
		{
			for (int32 i=index_start;i<out->Length();i++)
			{
				if (out->ByteAt(i) == character_start)
				{
					cpt_balance++;
				}
				else
					if (out->ByteAt(i) == character_end)
					{
						cpt_balance--;
					}

//				printf("%ld [%c] %ld -%s-\n",i,out->ByteAt(i),cpt_balance,out->String());
				if (cpt_balance == 0)
				{
					out->Remove(i+1,out->Length()-i-1);	// cut end
//					out->Remove(0,1);						// remove first character
					return;
				}
			}
		}
	}
}

void cut_string(BString *string,char charactere)
{
	if (string)
	{
		int32 index = string->FindFirst(charactere);
		if (index >= 0)
		{
			string->Remove(0,index+1);
			int32 index2 = string->FindFirst(charactere);
			if (index2 >= 0)
				string->Remove(index2,string->Length()-index2);
		}
	}
}

void extract_recipient_entry(BString *string,MDMailHeader *header, const char *fieldName)
{
	MDMailRecipient recipient;
	extract_recipient(string,&recipient);
	header->AddEntry(fieldName,recipient.email.String());
}

void extract_recipient(BString *string,MDMailRecipient *header)
{
	if (string)
	{
//		printf("extract_recipient : -%s-\n",string->String());

		int32 cpt_recipient = 0;
		while (string->Length() > 0)
		{
			// remove all leading ( and )
			while (string->ByteAt(0) == '(')
			{
				string->Remove(0,1);
			}
			// remove all leading ( and )
			while (string->ByteAt(string->Length()-1) == ')')
			{
				string->Remove(string->Length()-1,1);
			}
	
			// no Personal Name
			if (string->IFindFirst("NIL") == 0)
			{
				int32 index = string->FindFirst(' ');
				if (index >= 0)
				{
					string->Remove(0,index+1);
				}
				cpt_recipient++;
			}
			else
			{
				// extract value
				if (string->ByteAt(0) == '"')
				{
					string->Remove(0,1);
					int32 index = string->FindFirst('"');
					if (index >= 0)
					{
						char *pt = NULL;
						BString string3;
						string->CopyInto(string3,0,index);
						
						switch (cpt_recipient)
						{
							case 0:
								pt = rfc2047_to_utf8(string3.String());
								header->name = pt;
								free(pt);
								break;
							case 1:
								break;
							case 2:
								if (header->email.Length() == 0)
								{
									header->email = string3.String();
								}
								else
								{
									header->email.Prepend("@");
									header->email.Prepend(string3.String());
								}
								break;
							case 3:
								if (header->email.Length() == 0)
								{
									header->email = string3.String();
								}
								else
								{
									header->email.Append("@");
									header->email.Append(string3.String());
								}
								break;
							default:
								break;
						}
						
						if (string->ByteAt(index+2) != 0)
							string->Remove(0,index+2);
						else
							string->Remove(0,index+1);
					}
					cpt_recipient++;
					
					if (cpt_recipient == 4)
					{
						if (string->Length() > 0)
						{
							cpt_recipient = 0;
							string->Append(")");
						}
						else
							return;
					}
				}
			}
		}
	}
}

void extract_recipients_list_entry(BString *string,MDMailHeader *header, const char *fieldName)
{
	MDMailRecipientsList list;
	extract_recipients_list(string,&list);
	
	for (int32 i=0;i<list.CountItems();i++)
	{
		MDMailRecipient *pt = (MDMailRecipient *)list.ItemAt(i);
		if (pt)
		{
			header->AddEntry(fieldName,pt->email.String());
		}
	}
}

void extract_recipients_list(BString *string,MDMailRecipientsList *list)
{
	if ((string) && (*string != "NIL"))
	{
		BString string2(string->String());
		if (string2.ByteAt(0) == '(')
			string2.Remove(0,1);
		if (string2.ByteAt(string2.Length()-1) == ')')
			string2.Remove(string2.Length()-1,1);
//		printf("extract_recipients_list : -%s-\n",string2.String());
		
		MDMailRecipient *header = new MDMailRecipient();
		if (string)
		{
//			printf("extract_recipient : -%s-\n",string->String());
	
			int32 cpt_recipient = 0;
			while (string->Length() > 0)
			{
				// remove all leading ( and )
				while (string->ByteAt(0) == '(')
				{
					string->Remove(0,1);
				}
				// remove all leading ( and )
				while (string->ByteAt(string->Length()-1) == ')')
				{
					string->Remove(string->Length()-1,1);
				}
		
				// no Personal Name
				if (string->IFindFirst("NIL") == 0)
				{
					int32 index = string->FindFirst(' ');
					if (index >= 0)
					{
						string->Remove(0,index+1);
					}
					cpt_recipient++;
				}
				else
				{
					// extract value
					if (string->ByteAt(0) == '"')
					{
						string->Remove(0,1);
						int32 index = string->FindFirst('"');
						if (index >= 0)
						{
							char *pt = NULL;
							BString string3;
							string->CopyInto(string3,0,index);
							
							switch (cpt_recipient)
							{
								case 0:
									pt = rfc2047_to_utf8(string3.String());
									header->name = pt;
									free(pt);
									break;
								case 1:
									break;
								case 2:
									if (header->email.Length() == 0)
									{
										header->email = string3.String();
									}
									else
									{
										header->email.Prepend("@");
										header->email.Prepend(string3.String());
									}
									break;
								case 3:
									if (header->email.Length() == 0)
									{
										header->email = string3.String();
									}
									else
									{
										header->email.Append("@");
										header->email.Append(string3.String());
									}
									break;
								default:
									break;
							}
							
							if (string->ByteAt(index+2) != 0)
								string->Remove(0,index+2);
							else
								string->Remove(0,index+1);
						}
						cpt_recipient++;
						
						if (cpt_recipient == 4)
						{
							if (string->Length() > 0)
							{
								cpt_recipient = 0;
								string->Append(")");
								list->AddItem(header);
								header = new MDMailRecipient();
							}
							else
							{
								list->AddItem(header);
								break;
							}
						}
					}
				}
			}
		}
	}
}

void extract_parameter(BString *input,BString *output,int32 index)
{
	if ((input) && (output))
	{
		BString input2(*input);
	
		for (int32 i=0;i<=index;i++)
		{
			if (input2.ByteAt(0) == '(')
			{
				balance_string(input2.String(),output,'(',')');
				input2.Remove(0,output->Length()+1);
			}
			else
			{
				if (input2.IFindFirst("NIL") == 0)
				{
					input2.Remove(0,strlen("NIL")+1);
					*output = "NIL";
				}
			}
		}
	}
}

#pragma mark -

void extract_mono_block_parameters(BString *string,MDMailContainer *container)
{
	if ((string) && (container))
	{
		BString substring(string->String());
		int32 i=substring.FindFirst(')');
		if (i>0)
		{
			string->Remove(0,i+2);
			
			substring.Remove(i+1,substring.Length()-i-1);
			
			substring.Remove(0,1);
			if (substring.ByteAt(substring.Length()-1) == ')')
				substring.Remove(substring.Length()-1,1);

			while(substring.Length()>0)
			{
				BString out1;
				extract_mono_block_data(&substring,&out1,'"');
				if (out1.Length() > 0)
				{
					BString out2;
					extract_mono_block_data(&substring,&out2,'"');
					
					if (out1.ICompare("CHARSET") == 0)
					{
						container->charset = charset_to_utf8const(out2.String());
					}
					else
					if ((out1.ICompare("FILENAME") == 0) || (out1.ICompare("NAME") == 0))
						container->filename = out2.String();
					else
					{
						MAIL_CLIENT_IMAP_PRINT("extract_mono_block_parameters : unknown parameters : -%s-/-%s-\n",out1.String(),out2.String());
					}
					
					if (substring.ByteAt(0) == ' ')
						substring.Remove(0,1);
					if (substring.ByteAt(0) == '"')
						substring.Remove(0,1);
				}
			}
		}
	}
}

void extract_mono_block_data(BString *string, BString *out,char separator,char separator2)
{
	if ((string) && (out))
	{
		if (separator2 != -1)
		{
			if ((string->ByteAt(0) == separator) || (string->ByteAt(0) == separator2))
				string->Remove(0,1);
			int32 cpt=0;
			while((string->ByteAt(cpt) != separator) && (string->ByteAt(cpt) != separator2) && (string->ByteAt(cpt) != 0))
				*out+=string->ByteAt(cpt++);
			string->Remove(0,cpt+1);
			if (string->ByteAt(0) == ' ')
				string->Remove(0,1);
		}
		else
		{
			if (string->ByteAt(0) == separator)
				string->Remove(0,1);
			int32 cpt=0;
			while((string->ByteAt(cpt) != separator) && (string->ByteAt(cpt) != 0))
				*out+=string->ByteAt(cpt++);
			string->Remove(0,cpt+1);
			if (string->ByteAt(0) == ' ')
				string->Remove(0,1);
		}
	}
}

void extract_mono_block_data(BString *string,MDMail *mail, int32 section, int32 sub_section)
{
	if ((string) && (mail))
	{
		MDMailContainer *container = new MDMailContainer();
		
		if (string->ByteAt(0) == '(')
			string->Remove(0,1);
		if (string->ByteAt(string->Length()-1) == ')')
			string->Remove(string->Length()-1,1);
		
		// get the type
		if (string->IFindFirst("NIL") != 0)
		{
			extract_mono_block_data(string,&container->content_type,'"');
			MAIL_CLIENT_IMAP_PRINT("extract_mono_bloc_data : type = -%s-\n",container->content_type.String());
		}
		else
		{
			string->Remove(0,strlen("NIL")+1);
			MAIL_CLIENT_IMAP_PRINT("extract_mono_block_data : no type\n");
		}
		
		// get the sub-type
		if (string->IFindFirst("NIL") != 0)
		{
			extract_mono_block_data(string,&container->content_subtype,'"');
			MAIL_CLIENT_IMAP_PRINT("extract_mono_bloc_data : subtype = -%s-\n",container->content_subtype.String());
		}
		else
		{
			string->Remove(0,strlen("NIL")+1);
			MAIL_CLIENT_IMAP_PRINT("extract_mono_block_data : no sub-type\n");
		}
		
		// get the parameters
		if (string->IFindFirst("NIL") != 0)
		{
			BString out;
			extract_mono_block_parameters(string,container);
//			MAIL_CLIENT_IMAP_PRINT("extract_mono_bloc_data : string = -%s-\n",string->String());
		}
		else
		{
			string->Remove(0,strlen("NIL")+1);
			MAIL_CLIENT_IMAP_PRINT("extract_mono_block_data : no parameters\n");
		}
		
		// get the ID
		if (string->IFindFirst("NIL") != 0)
		{
			BString out;
			extract_mono_block_data(string,&out,'"');
			MAIL_CLIENT_IMAP_PRINT("extract_mono_bloc_data : ID = -%s-\n",out.String());
		}
		else
		{
			string->Remove(0,strlen("NIL")+1);
			MAIL_CLIENT_IMAP_PRINT("extract_mono_block_data : no ID\n");
		}
		
		// get the description
		if (string->IFindFirst("NIL") != 0)
		{
			BString out;
			extract_mono_block_data(string,&out,'"');
		}
		else
		{
			string->Remove(0,strlen("NIL")+1);
			MAIL_CLIENT_IMAP_PRINT("extract_mono_block_data : no description\n");
		}
		
		// get the encoding
		if (string->IFindFirst("NIL") != 0)
		{
			BString out;
			extract_mono_block_data(string,&out,'"');
			MAIL_CLIENT_IMAP_PRINT("extract_mono_bloc_data : encoding = -%s-\n",out.String());
			
			if (out.ICompare("X-UUENCODE") == 0)
				container->encoding = md_mail_encoding_uuencode;
				
			if (out.ICompare("BASE64") == 0)
				container->encoding = md_mail_encoding_base64;
			else
			if (out.ICompare("7BIT") == 0)
				container->encoding = md_mail_encoding_7bit;
			else
			if (out.ICompare("8BIT") == 0)
				container->encoding = md_mail_encoding_8bit;
		}
		else
		{
			string->Remove(0,strlen("NIL")+1);
			MAIL_CLIENT_IMAP_PRINT("extract_mono_block_data : no encoding\n");
		}
		
		// get the size
		if (string->IFindFirst("NIL") != 0)
		{
			BString out;
			extract_mono_block_data(string,&out,' ',')');
			MAIL_CLIENT_IMAP_PRINT("extract_mono_bloc_data : size = -%s-\n",out.String());
			container->size = (int64)atoi(out.String());
		}
		else
		{
			string->Remove(0,strlen("NIL")+1);
			MAIL_CLIENT_IMAP_PRINT("extract_mono_block_data : no size\n");
		}
		
		// conditionnal parameters
		if (container->content_type.ICompare("TEXT") == 0)
		{
			BString out;
			extract_mono_block_data(string,&out,')');
			MAIL_CLIENT_IMAP_PRINT("extract_mono_bloc_data : nb lines = -%s-\n",out.String());
		}
		if (container->content_type.ICompare("MESSAGE") == 0)
		{
		}
		
		// section
		char buffer_section[255];
		if (sub_section == 0L)
			sprintf(buffer_section,"%ld",section);
		else
			sprintf(buffer_section,"%ld.%ld",section,sub_section);
		container->section = buffer_section;
		
		mail->AddItem(container);
	}
}

#pragma mark -

void extract_dual_block_data(BString *string,MDMail *mail)
{
	MAIL_CLIENT_IMAP_PRINT("extract_dual_block_data\n");
	if ((string) && (mail))
	{
		if (string->ByteAt(0) == '(')
			string->Remove(0,1);
		if (string->ByteAt(string->Length()-1) == ')')
			string->Remove(string->Length()-1,1);

		int32 index = 1;
		while(string->FindFirst(')') >= 0)
		{
			if (string->FindFirst('(') < string->FindFirst('\"'))
				extract_mono_block_data(string,mail,index++);
			else
				break;
		}
	}
}

void extract_triple_block_data(BString *string,MDMail *mail, bool multiple_parameters)
{
	if ((string) && (mail))
	{
		if (string->ByteAt(0) == '(')
			string->Remove(0,1);
		if (!multiple_parameters)
			if (string->ByteAt(string->Length()-1) == ')')
				string->Remove(string->Length()-1,1);
			
		if (string->ByteAt(0) == '(')
			string->Remove(0,1);
		if (!multiple_parameters)
			if (string->ByteAt(string->Length()-1) == ')')
				string->Remove(string->Length()-1,1);
			
		// extract parts
		int32 sub_index = 1;
		while((string->ByteAt(0L) != '"') && (string->Length() > 0))
		{
			extract_mono_block_data(string,mail,1,sub_index++);
		}

		// ALTERNATIVE or MULTIPART ?
		if (string->ByteAt(0L) == '"')
		{
			string->Remove(0,1);
			int32 i=string->FindFirst('"');
			if (i>0)
			{
				BString sub_string(string->String(),i);
				string->Remove(0,sub_string.Length()+2);
			}

			int32 index = 2;
			while(string->FindFirst(')') >= 0)
			{
				extract_mono_block_data(string,mail,index++);
				if (multiple_parameters)
				{
					if ((string->ByteAt(0L) == '"') && ((string->IFindFirst("MIXED") == 1) || (string->IFindFirst("ALTERNATIVE") == 1)))
					{
						return;
					}
				}
			}
		}
	}
}

