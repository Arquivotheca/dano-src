// NameDialog.h

#ifndef NAME_DIALOG_H
#define NAME_DIALOG_H

#include <Window.h>

class NameDialog : public BWindow
{
public:
				NameDialog(BRect frame,const char *label, 
					const char *text, BMessage *modl, BHandler *target,
					const char *helpText = NULL);
virtual bool	QuitRequested();
virtual void	MessageReceived(BMessage *msg);

private:
	BHandler	*fTarget;
	BMessage 	*fModelMessage;
};

#endif
