#ifndef __MAIL_IMAP_ANSWERLIST_H__
#define __MAIL_IMAP_ANSWERLIST_H__

#include <List.h>

class MailIMAPAnswerList : public BList
{
public:
			MailIMAPAnswerList();
virtual		~MailIMAPAnswerList();
		
int32		Search(const char *);
void		Remove(int32);
void		PrintToStream();
void		Empty();
void		SortHeadersStructure();
void		SortHeadersStructureMult();

bool		get_parameter(const char *,BString *);

int32		cnt_answer;
};

#endif
