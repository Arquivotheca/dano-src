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

#include "MDContactAddonDefs.h"
#include "MDContactAddon.h"
#include "MDContact.h"
#include "MDContactList.h"
#include "MDAddressbooksList.h"

#include "ContactLOCALClient.h"
#include "ContactLOCALClientData.h"

MDContactAddon* instantiate_contact_daemon_addon(image_id id)
{
	MDContactAddon* addon = new MDContactAddon(id);
  	return addon;
}

#pragma mark -

MDContactAddon::MDContactAddon(image_id id)
{
	CONTACT_CLIENT_LOCAL_PRINT("ContactLOCALClient::ContactLOCALClient\n");
	protocol_data = new ContactLOCALClientData();
	fServer = NULL;
	fServerPort = 25;
	fLogin = NULL;
	fPassword = NULL;
	connected = false;
}

MDContactAddon::~MDContactAddon()
{
	CONTACT_CLIENT_LOCAL_PRINT("ContactLOCALClient::~ContactLOCALClient\n");
	if (protocol_data)
		delete protocol_data;
	if (fServer)
		delete[] fServer;
	if (fLogin)
		delete[] fLogin;
	if (fPassword)
		delete[] fPassword;
}

uint32 MDContactAddon::GetCapabilities()
{
//	return (md_contactaddon_capability_login_logout | md_contactaddon_capability_mailbox_listing | md_contactaddon_capability_data_sending);
	return 0L;
}

#pragma mark -

status_t MDContactAddon::Connect(const char *server, uint16 port, bool secured)
{
	CONTACT_CLIENT_LOCAL_PRINT("ContactLOCALClient::Connect\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if (data)
	{
		connected = true;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

bool MDContactAddon::IsConnected()
{
	return connected;
}

status_t MDContactAddon::Login(const char *login, const char *password, md_contactaddon_authentication authentication)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::Login\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if (data)
		return B_NO_ERROR;
	return B_ERROR;
}

status_t MDContactAddon::Logout()
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::Logout\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if (data)
		return B_NO_ERROR;
	return B_ERROR;
}

#pragma mark -

status_t MDContactAddon::SetAddressbookRoot(const char *path)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::SetAddressbookRoot : -%s-\n",path);
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if (data)
	{
		data->root = path;
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDContactAddon::SelectAddressbook(const char *name)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::SelectAddressbook : -%s-\n",name);
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((name) && (data) && (data->root.Length() > 0))
	{
		BString path(data->root.String());
		if (path.Length() > 0)
			path+="/";
		path+=name;
		
		if (path != data->selected_addressbook.String())
		{
			data->contact_list.SetRoot(path.String());
			data->selected_addressbook = path.String();
			return data->contact_list.LoadList();
		}
		else
			return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDContactAddon::DeselectAddressbook()
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::DeselectAddressbook\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if (data)
	{
		data->contact_list.SaveList();
		data->contact_list.Empty();
		data->selected_addressbook = "";
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDContactAddon::GetAddressbookStats(const char *name, uint32 *total)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::GetMailboxStats\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0) && (total))
	{
		status_t err = SelectAddressbook(name);
		if (err == B_NO_ERROR)
		{
			*total = data->contact_list.CountItems();
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

#pragma mark -

status_t MDContactAddon::GetAddressbooksList(MDAddressbooksList *list)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::GetAddressbooksList\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0))
	{
		BDirectory dir(data->root.String());
		if (dir.InitCheck() == B_NO_ERROR)
		{
			data->GetAddressbooksList(dir,list);
			return B_NO_ERROR;
		}
	}
	return B_ERROR;
}

status_t MDContactAddon::CreateAddressbook(const char *ref, const char *name)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::CreateAddressbook\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
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

status_t MDContactAddon::DeleteAddressbook(const char *name)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::DeleteAddressbook\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
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

status_t MDContactAddon::RenameAddressbook(const char *old_name, const char *new_name)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::RenameAddressbook\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
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

#pragma mark -

status_t MDContactAddon::AddContact(MDContact *contact)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::AddContact\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((data) && (contact) && (data->root.Length() > 0))
	{
		data->contact_list.AddContact(contact);
		data->contact_list.SetDirty();
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDContactAddon::DeleteContact(int64 id)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::DeleteContact\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0))
	{
		status_t err = data->contact_list.RemoveContact(id);
		if (err == B_NO_ERROR)
			data->contact_list.SetDirty();
		return err;
	}
	return B_ERROR;
}

status_t MDContactAddon::ModifyContact(int64 id, MDContact *contact)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::ModifyContact\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((data) && (contact) && (data->root.Length() > 0))
	{
		MDContact *foundcontact = data->contact_list.FindContact(id);
		if (foundcontact)
		{
			foundcontact->SetTo(*contact);
		}
		data->contact_list.SetDirty();
	}
	return B_ERROR;
}

// query functions
status_t MDContactAddon::QueryContactStart(const BMessage *parameters)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::QueryContactStart\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((data) && (data->root.Length() > 0))
	{
		data->index = 0;
		if (parameters)
			data->parameters = *parameters;
		else
			data->parameters.MakeEmpty();
		return B_NO_ERROR;
	}
	return B_ERROR;
}

status_t MDContactAddon::QueryContact(MDContactList *list, bool *done, int32 nb)
{
	CONTACT_CLIENT_LOCAL_PRINT("MailLOCALClient::QueryContact\n");
	ContactLOCALClientData *data = (ContactLOCALClientData *)protocol_data;
	if ((data) && (list) && (done) && (data->root.Length() > 0))
	{
		int32 cnt = 0;
		do
		{
			MDContact *contact = (MDContact *)data->contact_list.ItemAt(data->index);
			if ((contact) && (data->Match(contact)))
			{
				// filter the contact to keep the needed fields
				data->Filter(contact);
				list->AddItem(new MDContact(*contact));

				if (data->parameters.HasInt64(MD_CONTACT_ADDON_PREDICATE_ID))
				{
					*done = true;
					return B_NO_ERROR;
				}
			}
			data->index++;
			cnt++;
			
			if (cnt == nb)
			{
				*done = false;
				return B_NO_ERROR;
			}
		}
		while(data->index < (data->contact_list.CountItems()-1));
		*done = true;
		return B_NO_ERROR;
	}
	return B_ERROR;
}
