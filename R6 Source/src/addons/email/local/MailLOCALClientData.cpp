#include <SupportDefs.h>
#include <stdio.h>
#include <stdlib.h>

#include <String.h>
#include <Directory.h>
#include <Entry.h>
#include <Path.h>

#include "MDMailboxesList.h"
#include "MDMailbox.h"
#include "MDMailList.h"

#include "MailLOCALClient.h"
#include "MailLOCALClientData.h"

MailLOCALClientData::MailLOCALClientData()
{
	read_mail = NULL;
	index_start = 0;
	index_end = 0;
	index = 0;
}

MailLOCALClientData::~MailLOCALClientData()
{
}

void MailLOCALClientData::PrintToStream()
{
	printf("MailLOCALClientData::PrintToStream\n");
}

void MailLOCALClientData::GetMailboxesList(BDirectory dir,MDMailboxesList *list)
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
				list->AddItem(new MDMailbox(path_string.String()));
				
				BDirectory dir2(&entry);
				GetMailboxesList(dir2,list);
			}
		}
	}
}

