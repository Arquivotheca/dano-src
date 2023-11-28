//	MAlert.h
//	Copyright 1995 Metrowerks Corporation. All rights reserved.
//	Jon Watte
//
//	This handy set of utility classes can be used to simplify BAlerts.
//	It will make sure that the buttons are numbered in the RIGHT order,
//	which is from right to left, and that the escape key will select
//	button 2 (if there is one)
//
//	Also, buttons are numbered starting at one (1) and you can construct
//	an alert with no button argument, making for a default "OK" button.
//
//	You can also "run" the alert by comparing it to an integer, and you
//	can test its value by repeatedly calling Go() or operator int().
//
//	Each subclass creates an alert with a different icon.

//	[John Dance added]
//	The UI Guidelines say the cancel button should be on the left.
//	With the right-to-left numbering, this means the cancel button
// 	could be #2 or #3, depending on the number of buttons.
//	Rather than change this and change all calls to MAlert and
//	create bugs, I modified only two things:
//	1. I added another subclass MSaveFileAlert that has its
//	own button definitions
//	2. I changed the setting of the cancel button shortcut to
//	be the max number of buttons.  (In this way, an escape will
//	also dismiss an alert with just an ok, as it should.)

#ifndef _MALERT_H
#define _MALERT_H

#include <Alert.h>


enum AlertReplies {
	kOKButton = 1
};

class MAlert
{
		BAlert *				fAlert;
		int32					fResult;
		int32					fNumButtons;
static	bool					sAlertIsNoisy;
protected:
		void					MakeAlert(
									const char * message,
									const char * button1,
									const char * button2,
									const char * button3,
									alert_type type);
								MAlert();
public:
								MAlert(
									const char * message,
									const char * button1 = NULL,
									const char * button2 = NULL,
									const char * button3 = NULL);
								~MAlert();
		int32					Go();
		void					SetShortcut(
									int32 button,
									char key);
								operator int();
		void					AddFilter(
									BMessageFilter *filter);

static	bool					GetAlertIsNoisy();
static	void					SetAlertIsNoisy(
									bool noisy);
};

typedef MAlert MInfoAlert;


class MBlankAlert :
	public MAlert
{
public:
								MBlankAlert(
									const char * message,
									const char * button1 = NULL,
									const char * button2 = NULL,
									const char * button3 = NULL);
};


class MIdeaAlert :
	public MAlert
{
public:
								MIdeaAlert(
									const char * message,
									const char * button1 = NULL,
									const char * button2 = NULL,
									const char * button3 = NULL);
};


class MWarningAlert :
	public MAlert
{
public:
								MWarningAlert(
									const char * message,
									const char * button1 = NULL,
									const char * button2 = NULL,
									const char * button3 = NULL);
};


class MStopAlert :
	public MAlert
{
public:
								MStopAlert(
									const char * message,
									const char * button1 = NULL,
									const char * button2 = NULL,
									const char * button3 = NULL);
};


class MSaveFileAlert : public MAlert
{
public:
	enum SaveFileAlertReplies {
		kSaveButton = 1,
		kDontSaveButton = 2,
		kCancelButton = 3
	};

	MSaveFileAlert(
				   const char * message,
				   const char * button1,
				   const char * button2,
				   const char * button3);
};


#endif
