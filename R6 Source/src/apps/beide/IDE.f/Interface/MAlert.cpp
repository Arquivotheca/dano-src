// MAlert.cpp
// Utility class for sane-ifying Be alerts
// (c) Copyright 1995 Metrowerks Corporation. All rights reserved.
//	Jon Watte

#include "MAlert.h"

#include <Beep.h>



bool MAlert::sAlertIsNoisy = FALSE;

bool
MAlert::GetAlertIsNoisy()
{
	return sAlertIsNoisy;
}


void
MAlert::SetAlertIsNoisy(
	bool noisy)
{
	sAlertIsNoisy = noisy;
}


void
MAlert::MakeAlert(
	const char * message,
	const char * button1,
	const char * button2,
	const char * button3,
	alert_type type)
{
	const char * a1, *a2, *a3;
	if (button3)
	{
		fNumButtons = 3;
		a1 = button3;
		a2 = button2;
		a3 = button1;
	}
	else if (button2)
	{
		fNumButtons = 2;
		a1 = button2;
		a2 = button1;
		a3 = NULL;
	}
	else if (button1)
	{
		fNumButtons = 1;
		a1 = button1;
		a2 = NULL;
		a3 = NULL;
	}
	else
	{
		fNumButtons = 1;
		a1 = "OK";
		a2 = NULL;
		a3 = NULL;
	}
	// Create the real BAlert...
	// If we are working with 3 buttons, offset the left one.
	fAlert = new BAlert("Alert", 
						message, 
						a1, 
						a2, 
						a3,
						B_WIDTH_AS_USUAL, 
						fNumButtons == 3 ? B_OFFSET_SPACING : B_EVEN_SPACING, 
						type);
	// The cancel button should always be left-most.
	// This means it is always the highest number button (right-to-left)
	SetShortcut(fNumButtons, B_ESCAPE);	//	Escape
}


MAlert::MAlert()
{
	fAlert = NULL;
	fResult = 0;
	fNumButtons = 0;
}


MAlert::MAlert(
	const char * message,
	const char * button1,
	const char * button2,
	const char * button3)
{
	MakeAlert(
		message, button1, button2, button3,
		B_INFO_ALERT);
}


MAlert::~MAlert()
{
	delete fAlert;
}


int32
MAlert::Go()
{
	if (fAlert)
	{
		if (sAlertIsNoisy)
			beep();
		fResult = fNumButtons-fAlert->Go();
		fAlert = NULL;
	}
	return fResult;
}

void
MAlert::SetShortcut(
	int32 button,
	char key)
{
	if (fAlert && button > 0 && button <= fNumButtons) {
		fAlert->SetShortcut(fNumButtons - button, key);
	}
}


MAlert::operator int()
{
	return Go();
}

void
MAlert::AddFilter(
	BMessageFilter *filter)
{
	if (fAlert && fAlert->Lock())
	{
		fAlert->AddFilter(filter);
		fAlert->Unlock();
	}
}


MBlankAlert::MBlankAlert(
	const char * message,
	const char * button1,
	const char * button2,
	const char * button3)
{
	MakeAlert(
		message, button1, button2, button3,
		B_EMPTY_ALERT);
}


MIdeaAlert::MIdeaAlert(
	const char * message,
	const char * button1,
	const char * button2,
	const char * button3)
{
	MakeAlert(
		message, button1, button2, button3,
		B_IDEA_ALERT);
}


MWarningAlert::MWarningAlert(
	const char * message,
	const char * button1,
	const char * button2,
	const char * button3)
{
	MakeAlert(
		message, button1, button2, button3,
		B_WARNING_ALERT);
}


MStopAlert::MStopAlert(
	const char * message,
	const char * button1,
	const char * button2,
	const char * button3)
{
	MakeAlert(
		message, button1, button2, button3,
		B_STOP_ALERT);
}


MSaveFileAlert::MSaveFileAlert(
	const char * message,
	const char * button1,
	const char * button2,
	const char * button3)
{
	MakeAlert(message, button1, button2, button3, B_INFO_ALERT);
}
