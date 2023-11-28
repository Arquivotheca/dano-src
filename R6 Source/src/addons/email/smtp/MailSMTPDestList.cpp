#include <SupportDefs.h>
#include <stdio.h>
#include <string.h>

#include <String.h>

#include "MailSMTPDestList.h"
#include "MailSMTPClient.h"

MailSMTPDestListItem::MailSMTPDestListItem(const char *n)
{
	email=n;
	domain=n;
	get_reverse_domain();
	dont_use= false;
}

void MailSMTPDestListItem::PrintToStream()
{
	printf ("email = -%s-\n",email.String());
	printf ("domain = -%s-\n",domain.String());
	printf ("dont_use = %d\n",dont_use);
}

void MailSMTPDestListItem::get_domain()
{
	// extract the domain
	int32 index = domain.FindFirst("@");
	if (index >= 0)
		domain.Remove(0,index+1);
	
	domain.RemoveAll("[");
	domain.RemoveAll("]");
}

void MailSMTPDestListItem::get_reverse_domain()
{
	get_domain();

	// reverse the domain
	BString reversed_domain;
	
	bool end = false;
	do
	{
		// search for '.'
		int32 index = domain.FindLast('.');
		
		// cut the string
		if (index > 0)
		{
			BString tmp = domain.String();
			tmp.Remove(0,index+1);
			domain.Remove(index,domain.Length()-index);
			if (tmp.Length() > 0)
			{
				reversed_domain+=tmp.String();
				reversed_domain+='.';
			}
		}
		else
		{
			if (domain.Length() > 0)
				reversed_domain+=domain.String();
			end = true;
		}
	}
	while(end == false);
	domain.SetTo(reversed_domain);
}

#pragma mark -

MailSMTPDestList::MailSMTPDestList(const char *buffer)
	: BList()
{
	int32 index = 0;
	int32 len = 0;
	BString dest_field(buffer);
	dest_field.RemoveAll("<");
	dest_field.RemoveAll(">");
	
	while (index < dest_field.Length())
	{
		len = 0;
		while ((dest_field.ByteAt(index + len)) && (dest_field.ByteAt(index + len) != ','))
		{
			len++;
		}
		BString sub_string(dest_field.String()+index,len);

		MAIL_CLIENT_SMTP_PRINT("RCPT_TO : -%s-\n",sub_string.String());
		index += len + 1;
		add(sub_string.String());
	}
}

			


MailSMTPDestList::~MailSMTPDestList()
{
	Empty();
}

void MailSMTPDestList::Empty()
{
	for (int32 i=0;i<CountItems();i++)
		delete (MailSMTPDestListItem *)ItemAt(i);
	MakeEmpty();
}

void MailSMTPDestList::PrintToStream()
{
	for (int32 i=0;i<CountItems();i++)
	{
		MailSMTPDestListItem *item = (MailSMTPDestListItem *)ItemAt(i);
		if (item)
			item->PrintToStream();
	}
}

void MailSMTPDestList::add(const char *n)
{
	AddItem(new MailSMTPDestListItem(n));
}

void MailSMTPDestList::sort()
{
	if (CountItems() > 0)
		SortItems((int (*)(const void *, const void *))comp_nom);
}

int MailSMTPDestList::comp_nom(const MailSMTPDestListItem **p1,const MailSMTPDestListItem **p2)
{
	if ((p1) && (p2))
	{
		MailSMTPDestListItem *pt1 = (MailSMTPDestListItem *)*p1;
		MailSMTPDestListItem *pt2 = (MailSMTPDestListItem *)*p2;
		
		if ((pt1) && (pt2))
			return (strcmp(pt1->domain.String(),pt2->domain.String()));
	}
	return 0;
}

void MailSMTPDestList::remove_duplicate()
{
	for (int32 i=0;i<CountItems();i++)
	{
		MailSMTPDestListItem *item_1 = (MailSMTPDestListItem *)ItemAt(i);
		if (item_1)
		{
			for (int32 j=i+1;j<CountItems();j++)
			{
				MailSMTPDestListItem *item_2 = (MailSMTPDestListItem *)ItemAt(j);
				if (item_2)
				{
					printf ("MailSMTPDestList::eliminate_duplicate : %ld / %ld\n",i,j);
					if (item_1->email == item_2->email.String())
					{
						item_2->dont_use = true;
					}
				}
			}
		}
	}
}
