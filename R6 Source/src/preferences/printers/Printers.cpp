//
//	Printers.cpp	
//	Copyright 1998-2000 Be Incorporated, All Rights Reserved.
//

#include <Roster.h>
#include <Application.h>
#include <Resources.h>
#include <FindDirectory.h>
#include <File.h>
#include <Path.h>

// Needed for signatures
#include <pr_server.h>

#include "PrefPanel.h"


// Global object for accessing resouces
BResources AppResources;

// function to retrieve strings from the ressource fork
const char *RsrcGetString(int index)
{
	size_t size;
	const void *data = AppResources.LoadResource(B_STRING_TYPE, index, &size);
	return (const char *)data;
}


class TApplication : public BApplication
{
public:
	TApplication();
private:
	virtual void ReadyToRun();
	virtual void MessageReceived(BMessage *);
private:
	TWindow *tWindow;
	BPath setting_printer_dir;
};

// ---------------------------------------------------

TApplication::TApplication() : BApplication(PSRV_PRINT_PREFS_SIG)
{
	find_directory(B_USER_PRINTERS_DIRECTORY, &setting_printer_dir);
	
	app_info info;
	be_app->GetAppInfo(&info);
	BFile rsrc(&(info.ref), B_READ_ONLY);
	::AppResources.SetTo(&rsrc);
}

void TApplication::ReadyToRun(void)
{
	(tWindow = new TWindow(setting_printer_dir))->Show();
}

void TApplication::MessageReceived(BMessage *msg)
{
	switch(msg->what)
	{
		case msg_add_printer:
		case msg_remove_printer:
			tWindow->PostMessage(msg);
			break;
		default:
			BApplication::MessageReceived(msg);
			break;
	}
}

// ---------------------------------------------------

int main()
{
	TApplication app;
	app.Run();
	return 0;
}

