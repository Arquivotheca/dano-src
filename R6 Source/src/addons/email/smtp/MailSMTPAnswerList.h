#ifndef __MAIL_SMTP_ANSWERLIST_H__
#define __MAIL_SMTP_ANSWERLIST_H__

#include <List.h>

class MailSMTPAnswerList : public BList
{
public:
		MailSMTPAnswerList();
		~MailSMTPAnswerList();
		
int32	Search(const char *);
void	Remove(int32);
void	PrintToStream();
void	Empty();

bool	get_parameter(const char *,BString *);

};

#endif
