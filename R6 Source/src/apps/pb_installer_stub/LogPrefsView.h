/*
	
	HelloView.h
	
	Copyright 1995 Be Incorporated, All Rights Reserved.
	
*/
#ifndef _LOGPREFSVIEW_H_
#define _LOGPREFSVIEW_H_


#include "ResizeView.h"
#include "Log.h"


class LogPrefsView : public ResizeView {

public:
				LogPrefsView(BRect frame); 
virtual	void	AttachedToWindow();
virtual void 	MessageReceived(BMessage *msg);
virtual	void	Draw(BRect updateRect);

void 			setLog_Valet(bool v);
bool			getLog_Valet();

void 			setLog_Jaegar(bool v);
bool			getLog_Jaegar();

void 			setLog_Trolling(bool v);
bool 			getLog_Trolling();

void 			setLog_Installations(bool v);
bool			getLog_Installations();

void 			setLog_Uninstallations(bool v);
bool			getLog_Uninstallations();

void 			setLog_Registrations(bool v);
bool			getLog_Registrations();

void 			setLog_Downloads(bool v);
bool			getLog_Downloads();

void 			setLog_Updates(bool v);
bool			getLog_Updates();

void 			setLog_BeDepot(bool v);
bool			getLog_BeDepot();

void 			setNotification_freq(short v);
short			getNotification_freq();

enum {
	EMAIL_CONTROL		= 'ECon',
	LOG_CONTROL			= 'LCon'
};

private:
		int32	Log_flags;
		short 	notification_freq;
};


#endif