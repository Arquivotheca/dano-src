#include <SupportDefs.h>
#include <stdio.h>
#include <stdlib.h>

#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "MDAddressbooksList.h"
#include "MDAddressbook.h"

#include "ContactLOCALClient.h"
#include "ContactLOCALClientData.h"
#include "MDContactAddonDefs.h"
#include "MDContact.h"

ContactLOCALClientData::ContactLOCALClientData()
{
	index = 0;
}

ContactLOCALClientData::~ContactLOCALClientData()
{
}

void ContactLOCALClientData::PrintToStream()
{
	printf("ContactLOCALClientData::PrintToStream\n");
}

void ContactLOCALClientData::GetAddressbooksList(BDirectory dir,MDAddressbooksList *list)
{
	if (list)
	{
		int32 index = 0;
		BEntry entry;
		while (dir.GetNextEntry(&entry,index++) == B_NO_ERROR)
		{
			if (entry.IsDirectory())
			{
				// add item in the list
				BPath path;
				entry.GetPath(&path);
				BString path_string(path.Path());
				path_string.Remove(0,root.Length()+1);
				list->AddItem(new MDAddressbook(path_string.String()));
				
				BDirectory dir2(&entry);
				GetAddressbooksList(dir2,list);
			}
		}
	}
}

bool ContactLOCALClientData::Match(MDContact *contact)
{
	if (contact)
	{
		// check predicate (id testing)
		if (parameters.HasInt64(MD_CONTACT_ADDON_PREDICATE_ID))
		{
			int64 id = 0L;
			parameters.FindInt64(MD_CONTACT_ADDON_PREDICATE_ID,&id);
			if (contact->id == id)
				return true;
			else
				return false;
		}
		return true;
	}
	return false;
}

void ContactLOCALClientData::Filter(MDContact *contact)
{
	if ((contact) && (parameters.HasString(MD_CONTACT_ADDON_FIELD)))
	{
		status_t err = B_ERROR;
		int32 index = 0L;
		do
		{
			BString field;
			err = parameters.FindString(MD_CONTACT_ADDON_FIELD,index,&field);
			if (err == B_NO_ERROR)
			{
				contact->RemoveEntry(field.String());
			}
		}
		while(err == B_NO_ERROR);
	}
}
