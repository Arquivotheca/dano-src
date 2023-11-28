// ValetWindow.h

#ifndef _VALETWINDOW_H_
#define _VALETWINDOW_H_

#include <Window.h>
#include <Message.h>
#include <Messenger.h>
#include "ValetApp.h"

class SettingsManager;

class ValetWindow : public BWindow
{
public:
				ValetWindow();
virtual			~ValetWindow();
	
virtual void	MessageReceived(BMessage *m);
virtual bool	QuitRequested();

private:
friend class ValetApp;
	//SettingsManager	*settings;
	
	BMessenger			packWindow;
	BMessenger			downWindow;
	BMessenger			settWindow;
	BMessenger			manaWindow;
};

#endif
