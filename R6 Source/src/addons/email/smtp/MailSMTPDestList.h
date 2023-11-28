#ifndef __MailSMTPDestList_h__
#define __MailSMTPDestList_h__

#include <String.h>
#include <List.h>

class BString;
class MailSMTPDestListItem
{
public:
		MailSMTPDestListItem(const char *);

void 	PrintToStream();

	BString	email;
	BString	domain;
	bool	dont_use;
	
private:
void	get_domain();
void	get_reverse_domain();
};

class MailSMTPDestList : public BList
{
public:
		MailSMTPDestList(const char *buffer = NULL);
virtual	~MailSMTPDestList();
void 	Empty();
void 	PrintToStream();
	
void 	add(const char *);
void	sort();
void	remove_duplicate();

private:
static int 	comp_nom(const MailSMTPDestListItem **,const MailSMTPDestListItem **);

};

#endif
