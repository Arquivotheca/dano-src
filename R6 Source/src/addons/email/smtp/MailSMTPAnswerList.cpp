#include <stdio.h>

#include <String.h>

#include "MailSMTPAnswerList.h"

MailSMTPAnswerList::MailSMTPAnswerList()
{
}

MailSMTPAnswerList::~MailSMTPAnswerList()
{
	Empty();
}

void MailSMTPAnswerList::Empty()
{
	for (int32 i=0;i<CountItems();i++)
		delete (BString *)ItemAt(i);
	MakeEmpty();
}

int32 MailSMTPAnswerList::Search(const char *ch_a_rechercher)
{
	for (int32 i=0;i<CountItems();i++)
	{
		BString *ch = (BString *)ItemAt(i);
		if (ch->FindFirst(ch_a_rechercher) != -1)
			return i;
	}
	return -1;
}

void MailSMTPAnswerList::Remove(int32 index)
{
	delete (BString *)RemoveItem(index);
}

void MailSMTPAnswerList::PrintToStream()
{
	for (int32 i=0;i<CountItems();i++)
	{
		BString *ch = (BString *)ItemAt(i);
		if (ch)
			printf("-%s-\n",ch->String());
	}
}

bool MailSMTPAnswerList::get_parameter(const char *tag,BString *answer)
{
	*answer = "";
	for (int32 i=0;i<CountItems();i++)
	{
		BString *ch = (BString *)ItemAt(i);
		if (ch)
		{
			int32 index = ch->FindFirst(tag);
			if (index >= 0)
			{
				ch->Remove(0,index);
				if (ch->Length() > 0)
				{
					int32 index2 = ch->FindFirst(" ");
					if (index2 > 0)
					{
						ch->Remove(index2,ch->Length()-index2);
						if (ch->Length() > 0)
						{
							ch->Remove(0,strlen(tag)+1);
							*answer = *ch;
							return true;
						}
					}
				}
				else
					break;
			}
		}
		else
			break;
	}
	return false;
}
