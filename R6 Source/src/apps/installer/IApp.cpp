//******************************************************************************
//
//	File:			IApp.cpp
//
//	Description:	Be Installer application.
//
//	Written by:		Steve Horowitz
//
//	Copyright 1994-96, Be Incorporated. All Rights Reserved.
//
//******************************************************************************

#ifndef _DEBUG_H
#include <Debug.h>
#endif
#ifndef _ALERT_H
#include <Alert.h>
#endif

#ifndef IAPP_H
#include "IApp.h"
#endif

#include <string.h>

TIApp::TIApp()
	: BApplication("application/x-vnd.Be-INST")
{
}

TIApp::~TIApp()
{
	fEngine->Lock();
	fEngine->Quit();
}

bool TIApp::QuitRequested()
{
	BAlert*	alert;
	
	while (fEngine->State() == LOADING)
		snooze(10000);

	if (fEngine->State() == IDLE)
		return(inherited::QuitRequested());
	else {
		alert = new BAlert("", "The installer is busy, please wait before quitting.", "OK");
		alert->Go();
		return(FALSE);
	}
}

void TIApp::ReadyToRun()
{	
	fEngine = new TEngine();
	fEngine->Run();
	fEngine->PostMessage(INIT_ENGINE);
}

void 
TIApp::ArgvReceived(int32, char **argv)
{
	if (!*++argv)
		return;
		
	if (strcmp(*argv, "--showMessage") == 0) {
		if (!*++argv)
			return;
		// installer init script is asking us to show a message
		BMessage message(DISPLAY_MESSAGE);
		message.AddString("statusMessage", *argv);
		fEngine->Window()->PostMessage(&message);
	} else if (strcmp(*argv, "--showBarberPole") == 0) {
		// installer init script is asking us to show a message
		BMessage message(SHOW_BARBER_POLE);
		fEngine->Window()->PostMessage(&message);
	} else if (strcmp(*argv, "--hideBarberPole") == 0) {
		// installer init script is asking us to show a message
		BMessage message(HIDE_BARBER_POLE);
		fEngine->Window()->PostMessage(&message);
	}
}

void 
TIApp::MessageReceived(BMessage *message)
{
	switch (message->what) {
		case 'gsts':
			{
				BMessage reply('stat');
				reply.AddInt32("status", fEngine->State());
				message->SendReply(&reply);
			}
			break;

		default:
			BApplication::MessageReceived(message);
			return;
	}
}

int
main()
{
	TIApp*	app;
	
	setgid(0);
	setuid(0);
	
	app = new TIApp();
	app->Run();
	delete(app);

	return(0);
}
