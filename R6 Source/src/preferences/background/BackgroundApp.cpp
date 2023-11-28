//*****************************************************************************
//
//	File:		 BackgroundApp.cpp
//
//	Description: Application class for Background preference panel
//
//	Copyright 1996 - 1998, Be Incorporated
//
//*****************************************************************************

#include "BackgroundApp.h"
#include "SetupWin.h"

BackgroundApp::BackgroundApp(const char *sig)
 : BApplication(sig)
{
}

void BackgroundApp::ReadyToRun()
{
	SetupWin	*w;
	w = new SetupWin();
	w->Show();
}

void BackgroundApp::RefsReceived(BMessage *refs)
{
	if(IsLaunching())
	{
		// I'll look at it again later...
		PostMessage(refs);
	}
	else
	{
		BWindow	*w = WindowAt(0);
		if(w->Lock())
		{
			refs->what = B_SIMPLE_DATA;
			w->PostMessage(refs);
			w->Unlock();
		}
	}
}

