#include <stdio.h>
#include <stdlib.h>

#include <malloc.h>
#include <memory.h>
#include <OS.h>
#include <String.h>
#include <Directory.h>
#include <Path.h>
#include <File.h>
#include <bsd_mem.h>
#include <List.h>

#include "MDE-mail.h"
#include "MDMailAddonDefs.h"
#include "MDMailAddon.h"
#include "MDMailList.h"
#include "MDMail.h"
#include "MDMailboxesList.h"
#include "MDMailHeadersList.h"
#include "MDMailContainer.h"

#include "MailLOCALClient.h"
#include "MailLOCALClientData.h"

MDMailAddon* instantiate_mail_daemon_addon(image_id id)
{
	MDMailAddon* addon = new MDMailAddon(id);
  	return addon;
}

#pragma mark -

MDMailAddon::MDMailAddon(image_id id)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::MailLOCALClient\n");
	protocol_data = new MailLOCALClientData();
	fServer = NULL;
	fServerPort = 25;
	fLogin = NULL;
	fPassword = NULL;
	connected = false;
}

MDMailAddon::~MDMailAddon()
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::~MailLOCALClient\n");
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
	return (md_mailaddon_capability)(md_mailaddon_capability_login_logout | md_mailaddon_capability_mailbox_listing | md_mailaddon_capability_data_sending);
}

uint32 MDMailAddon::GetDirection()
{
	return (md_mailaddon_direction_incoming | md_mailaddon_direction_outgoing);
}

#pragma mark -

status_t MDMailAddon::Connect(const char *server, uint16 port, bool secured)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::Connect\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if (data)
		return B_NO_ERROR;
	return B_ERROR;
}

bool MDMailAddon::IsConnected()
{
	return connected;
}

status_t MDMailAddon::Login(const char *login, const char *password, md_mailaddon_authentication authentication)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::Login\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if (data)
		return B_NO_ERROR;
	return B_ERROR;
}

status_t MDMailAddon::Logout()
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::Logout\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if (data)
		return B_NO_ERROR;
	return B_ERROR;
}

status_t MDMailAddon::GetServerAvailableSpace(const char *mailbox, off_t *storage_current, off_t *storage_limit, int32 *message_current, int32 *message_limit)
{
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::SetMailboxesRoot(const char *path)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::SetMailboxesRoot : -%s-\n",path);
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if (data)
	{
		data->root = path;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailboxStats(const char *name, uint32 *total_messages, uint32 *new_messages)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailboxStats\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0) && (total_messages) && (new_messages))
	{
		status_t err = SelectMailbox(name);
		if (err == B_NO_ERROR)
		{
			data->mail_list.GetMailboxStats(total_messages,new_messages);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::SelectMailbox(const char *name)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::SelectMailbox : -%s-\n",name);
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((name) && (data) && (data->root.Length() > 0))
	{
		BString path(data->root.String());
		if (path.Length() > 0)
			path+="/";
		path+=name;
		
		if (path != data->selected_mailbox.String())
		{
			data->selected_mailbox = path.String();
			data->mail_list.SetRoot(path.String());
			return data->mail_list.LoadList();
		}
		else
			return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDMailAddon::SelectMailboxRO(const char *name)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::SelectMailboxRO : -%s-\n",name);
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((name) && (data) && (data->root.Length() > 0))
	{
		BString path(data->root.String());
		if (path.Length() > 0)
			path+="/";
		path+=name;
		if (path != data->selected_mailbox.String())
		{
			data->selected_mailbox = path.String();
			data->mail_list.SetRoot(path.String());
			return data->mail_list.LoadList();
		}
		else
			return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDMailAddon::DeselectMailbox()
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::DeselectMailbox\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if (data)
	{
		data->mail_list.SaveList();
		data->mail_list.Empty();
		data->selected_mailbox = "";
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDMailAddon::CleanupMailbox()
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::CleanupMailbox\n");
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailboxesList(MDMailboxesList *list, bool get_mailbox_stats)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailboxesList\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0))
	{
		BDirectory dir(data->root.String());
		if (dir.InitCheck() == B_NO_ERROR)
		{
			data->GetMailboxesList(dir,list);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::GetSubscribedMailboxesList(MDMailboxesList *list, bool get_mailbox_stats)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetSubscribedMailboxesList\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0))
	{
		BDirectory dir(data->root.String());
		if (dir.InitCheck() == B_NO_ERROR)
		{
			data->GetMailboxesList(dir,list);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::CreateMailbox(const char *ref, const char *name)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::CreateMailbox\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (ref) && (name) && (data->root.Length() > 0))
	{
		BDirectory dir(data->root.String());
		if (dir.InitCheck() == B_NO_ERROR)
		{
			BDirectory dir2;
			BString path(ref);
			if (path.Length() > 0)
				path+="/";
			path+=name;
			return dir.CreateDirectory(path.String(),&dir2);
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::DeleteMailbox(const char *name)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::DeleteMailbox\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (name) && (data->root.Length() > 0))
	{
		BString path(data->root.String());
		if (path.Length() > 0)
			path+="/";
		path+=name;
		
		BDirectory dir(path.String());
		if (dir.InitCheck() == B_NO_ERROR)
		{
			// check if the mailbox contains another folder, in this case fails !!!
			int32 index = 0;
			BEntry entry;
			while (dir.GetNextEntry(&entry,index++) == B_NO_ERROR)
			{
				if (entry.IsDirectory())
					return B_ERROR;
			}
			
			// delete all items
			dir.Rewind();
			index = 0;
			while (dir.GetNextEntry(&entry,index++) == B_NO_ERROR)
			{
				entry.Remove();
			}
			
			// delete the entry
			entry.SetTo(path.String());
			return entry.Remove();
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::RenameMailbox(const char *old_name, const char *new_name)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::RenameMailbox\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (old_name) && (new_name) && (data->root.Length() > 0))
	{
		BString path(data->root.String());
		if (path.Length() > 0)
			path+="/";
		path+=old_name;
		
		BEntry entry(path.String());
		if (entry.Exists())
		{
			return entry.Rename(new_name);
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::SubscribeMailbox(const char *name)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::SubscribeMailbox\n");
	return B_ERROR;
}

status_t MDMailAddon::UnsubscribeMailbox(const char *name)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::UnsubscribeMailbox\n");
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailHeaders(const char *index ,MDMailHeader *headers, const BMessage *needed_headers)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailHeaders\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((index) && (data) && (headers) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		MDMail *mail = (MDMail *)data->mail_list.ItemAt(atoi(index)-1);
		if (mail)
		{
			headers->SetTo(mail->header);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeadersStart(const char *index_start, const char *index_end, const BMessage *needed_headers)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailHeadersStart\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((index_start) && (index_end) && (data) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		int32 start = atoi(index_start);
		int32 end = atoi(index_end);
		if ((start == -1) && (end == -1))
		{
			start = 1;
			end = data->mail_list.CountItems();
		}
		data->index_start = start;
		data->index_end = end;
		data->index = start-1;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeaders(MDMailHeadersList *list, bool *done, int32 nb)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailHeaders\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (done) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		int32 cnt = 0;
		do
		{
			MDMail *mail = (MDMail *)data->mail_list.ItemAt(data->index);
			if (mail)
			{
				list->AddItem(new MDMailHeader(mail->header));
			}
			data->index++;
			cnt++;
			
			if (cnt == nb)
			{
				*done = false;
				return B_NO_ERROR;
			}
		}
		while(data->index < (data->index_end-1));
		*done = true;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailStructure(const char *index, MDMail *mail)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailStructure\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((index) && (data) && (mail) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		MDMail *pt_mail = (MDMail *)data->mail_list.ItemAt(atoi(index)-1);
		if (pt_mail)
		{
			mail->SetTo(*pt_mail);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailStructureStart(const char *index_start, const char *index_end)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailStructureStart\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((index_start) && (index_end) && (data) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		int32 start = atoi(index_start);
		int32 end = atoi(index_end);
		if ((start == -1) && (end == -1))
		{
			start = 1;
			end = data->mail_list.CountItems();
		}
		data->index_start = start;
		data->index_end = end;
		data->index = start-1;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailStructure(MDMailList *list, bool *done, int32 nb)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailStructure\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (done) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		list->SetRoot(data->selected_mailbox.String());
		int32 cnt = 0;
		do
		{
			MDMail *mail = (MDMail *)data->mail_list.ItemAt(data->index);
			if (mail)
			{
				MDMail *new_mail = new MDMail();
				new_mail->SetTo(*mail);
				list->AddItem(new_mail);
			}
			data->index++;
			cnt++;
			
			if (cnt == nb)
			{
				*done = false;
				return B_NO_ERROR;
			}
		}
		while(data->index < (data->index_end-1));
		*done = true;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailHeadersStructure(const char *index, MDMail *mail, const BMessage *needed_headers)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailHeadersStructure\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((index) && (data) && (mail) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		MDMail *pt_mail = (MDMail *)data->mail_list.ItemAt(atoi(index)-1);
		if (pt_mail)
		{
			mail->header.SetTo(pt_mail->header);
			mail->SetTo(*pt_mail);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeadersStructureStart(const char *index_start, const char *index_end, const BMessage *needed_headers = NULL)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailHeadersStructureStart\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		int32 start = atoi(index_start);
		int32 end = atoi(index_end);
		if ((start == -1) && (end == -1))
		{
			start = 1;
			end = data->mail_list.CountItems();
		}
		data->index_start = start;
		data->index_end = end;
		data->index = start-1;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailHeadersStructure(MDMailList *list, bool *done, int32 nb)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailHeadersStructure\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((done) && (data) && (list) && (data->root.Length() > 0) && (data->mail_list.CountItems() > 0))
	{
		list->SetRoot(data->selected_mailbox.String());
		int32 cnt = 0;
		do
		{
			MDMail *mail = (MDMail *)data->mail_list.ItemAt(data->index);
			if (mail)
			{
				MDMail *new_mail = new MDMail();
				new_mail->header.SetTo(mail->header);
				new_mail->SetTo(*mail);
				list->AddItem(new_mail);
			}
			data->index++;
			cnt++;
			
			if (cnt == nb)
			{
				*done = false;
				return B_NO_ERROR;
			}
		}
		while(data->index < (data->index_end-1));
		*done = true;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::SendData(MDMail *mail, const char *folder)
{
	MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::SendData\n");
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((mail) && (data))
	{
		// select the requested folder
		status_t err = B_NO_ERROR;
		mail->SetRoot(data->selected_mailbox.String());
		MAIL_CLIENT_LOCAL_PRINT("MailLOCALClient::SendData : selected_mailbox = -%s-\n",data->selected_mailbox.String());
		
		for(int32 i=0;i<mail->CountItems();i++)
		{
			MDMailContainer *container = (MDMailContainer *)mail->ItemAt(i);
			if (container)
			{
				mail->AddContentStart(container);
				BMallocIO data;
				data.Write(container->data,container->size);
				data.Seek(0,SEEK_SET);
				err = mail->AddContent(&data,container);
				err = mail->AddContentEnd(container);
			}
		}
			
		data->mail_list.AddItem(mail);
		data->mail_list.SetDirty();
		
		data->mail_list.SaveList();
		data->mail_list.MakeEmpty();
		err = DeselectMailbox();
	}
	else
		printf("MailLOCALClient::SendData : argh 2\n");
	return B_ERROR;
}

#pragma mark -

status_t MDMailAddon::GetMailContentStart(const char *mail_id, const char *section)
{
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (mail_id) && (data->selected_mailbox != "") && (section))
	{
		data->read_mail = (MDMail *)data->mail_list.ItemAt(atoi(mail_id));
		if (data->read_mail)
		{
			data->read_mail->SetRoot(data->selected_mailbox.String());
			MDMailContainer *container = data->read_mail->FindSection(section);
			if (container)
			{
				int64 size = 0;
				return(data->read_mail->GetContentStart(container,&size));
			}
		}
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailContent(MDMailContainer *container,BMallocIO *out_buffer, int64 buffer_size, int64 *size, bool *done)
{
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((container) && (data) && (buffer_size > 0) && (size) && (done) && (data->selected_mailbox != "") && (out_buffer))
	{
		out_buffer->SetSize(0L);
		return(data->read_mail->GetContent(out_buffer,container,buffer_size,done));
	}
	return B_ERROR;
}

status_t MDMailAddon::GetMailContentEnd()
{
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if (data)
	{
		status_t err = data->read_mail->GetContentEnd();
		data->read_mail = NULL;
		return err;
		
	}
	return B_NO_ERROR;
}

#pragma mark -

status_t MDMailAddon::DeleteMail(const char *uid)
{
	MailLOCALClientData *data = (MailLOCALClientData *)protocol_data;
	if ((data) && (uid) && (data->selected_mailbox != ""))
	{
		MDMail *mail = (MDMail *)data->mail_list.FindMail(uid);
		if (mail)
		{
			mail->SetRoot(data->root.String());
			if (mail->Delete() == B_NO_ERROR)
			{
				data->mail_list.RemoveItem(mail);
				delete mail;
				data->mail_list.SetDirty();
				return B_NO_ERROR;
			}
			else
				printf("MDMailAddon::DeleteMail : error deleting mail\n");
		}
	}
	return B_ERROR;
}

