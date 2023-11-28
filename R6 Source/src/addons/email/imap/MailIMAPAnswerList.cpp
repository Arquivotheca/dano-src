#include <stdio.h>

#include <String.h>

#include "MailIMAPAnswerList.h"

MailIMAPAnswerList::MailIMAPAnswerList()
	: BList()
{
	cnt_answer = -1L;
}

MailIMAPAnswerList::~MailIMAPAnswerList()
{
	Empty();
}

int32 MailIMAPAnswerList::Search(const char *ch_a_rechercher)
{
	for (int32 i=0;i<CountItems();i++)
	{
		BString *ch = (BString *)ItemAt(i);
		if (ch->FindFirst(ch_a_rechercher) != -1)
			return i;
	}
	return -1;
}

void MailIMAPAnswerList::Empty()
{
	for (int32 i=0;i<CountItems();i++)
		delete (BString *)ItemAt(i);
	MakeEmpty();
}

void MailIMAPAnswerList::Remove(int32 index)
{
	delete (BString *)RemoveItem(index);
}

void MailIMAPAnswerList::PrintToStream()
{
	for (int32 i=0;i<CountItems();i++)
	{
		BString *ch = (BString *)ItemAt(i);
		if (ch)
			printf("-%s-\n",ch->String());
	}
}

bool MailIMAPAnswerList::get_parameter(const char *tag,BString *answer)
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
					else
					{
						ch->Remove(0,strlen(tag)+1);
						*answer = *ch;
						return true;
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

// sorting function to put :
//		- IMAP
//		- HEADERS
void MailIMAPAnswerList::SortHeadersStructure()
{
	bool all_headers_received = false;
	for (int32 i=0;i<CountItems();i++)
	{
		BString *ch = (BString *)ItemAt(i);
		if (ch)
		{
			// IMAP answer (do nothing)
			if (ch->ByteAt(0) == '*')
			{
			}
			else
			{
				// Receiving headers
				if (!all_headers_received)
				{
					if (*ch == "")
						all_headers_received = true;
					else
						if (cnt_answer == -1)
							cnt_answer = i;
				}
				else
				{
					// We got a answer at the wrong place, so move everything up
//					printf("MailIMAPAnswerList::SortHeadersStructure : %ld -%s-\n",cnt_answer,ch->String());
					BString *newstring = new BString(ch->String());
					Remove(i);
					AddItem(newstring,cnt_answer);
					cnt_answer++;
				}
			}
		}
	}
}


// sorting function to put :
//		- IMAP
//		- HEADERS
void MailIMAPAnswerList::SortHeadersStructureMult()
{
	bool all_headers_received = false;
	for (int32 i=0;i<CountItems();i++)
	{
		BString *ch = (BString *)ItemAt(i);
		if (ch)
		{
			// IMAP answer (do nothing)
			if (ch->ByteAt(0) == '*')
			{
				all_headers_received = false;
				cnt_answer = -1L;
			}
			else
			{
				// Receiving headers
				if (!all_headers_received)
				{
					if (*ch == "")
						all_headers_received = true;
					else
						if (cnt_answer == -1)
							cnt_answer = i;
				}
				else
				{
					// We got a answer at the wrong place, so move everything up
//					printf("MailIMAPAnswerList::SortHeadersStructure : %ld -%s-\n",cnt_answer,ch->String());
					BString *newstring = new BString(ch->String());
					Remove(i);
					AddItem(newstring,cnt_answer);
					cnt_answer++;
				}
			}
		}
	}
}
