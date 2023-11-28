/******************************************************************************

	File:			shex.cpp

	Description:	Script Launcher application.

	Written by:		Steve Horowitz

	Copyright 1994-95, Be Incorporated. All Rights Reserved.

*****************************************************************************/

#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#include <Debug.h>

#include <OS.h>
#include <Application.h>
#include <Path.h>
#include <image.h>
#include <Window.h>
#include <View.h>
#include <Directory.h>
#include <File.h>

class TStatusView : public BView {
public:
					TStatusView(BRect);
virtual void		Draw(BRect);
};

class TStatusWindow : public BWindow {
public:
					TStatusWindow();
};

class TScriptApp : public BApplication {

friend class TStatusView;

public:
		 			TScriptApp();
virtual	void		RefsReceived(BMessage*);
virtual void		ReadyToRun();

		entry_ref	fScriptRef;
};

BRect r(100, 100, 275, 135);

extern "C" load_posix(const char *, const char **, const char **);

// ---------------------------------------------------------------

TStatusView::TStatusView(BRect r)
		    :BView(r, "", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED)
{
}

// ---------------------------------------------------------------

void TStatusView::Draw(BRect)
{
	char	buf[100];

	SetFontSize(9);
	MovePenTo(10, 20);
	sprintf(buf, "Running Script \"%s\"", ((TScriptApp*)be_app)->fScriptRef.name);
	DrawString(buf);
}

// ---------------------------------------------------------------

TStatusWindow::TStatusWindow()
			  :BWindow(r, "Script Status", B_TITLED_WINDOW,
					   B_NOT_CLOSABLE | B_NOT_RESIZABLE | B_NOT_ZOOMABLE)
{
	r.OffsetTo(B_ORIGIN);
	AddChild(new TStatusView(r));
	Show();
}

// ---------------------------------------------------------------

TScriptApp::TScriptApp()
		   :BApplication("application/x-vnd.Be-cmd-SCPT")
{
}

//--------------------------------------------------------------------

extern char** environ;

void TScriptApp::RefsReceived(BMessage* msg)
{
	BPath				path;
	thread_info			info;
	thread_id			tid;
	TStatusWindow*		wind;
	BEntry				entry;
	BEntry				parent_entry;
	BDirectory			dir;
	char*				eargv[3];
	ulong				type;
	long				count;
	long				i;
	long				result;

	if (!msg->GetInfo("refs", &type, &count))
		return;

	for (i = 0; i < count; i++) {
		if (msg->FindRef("refs", i, &fScriptRef) == B_NO_ERROR) {
			if (entry.SetTo(&fScriptRef) == B_NO_ERROR) {
				entry.GetParent(&dir);
				dir.GetEntry(&parent_entry);

				eargv[0] = "/bin/sh";
				eargv[1] = fScriptRef.name;
				eargv[2] = 0;
				parent_entry.GetPath(&path);
				chdir(path.Path());
				tid = load_image(2, eargv, environ);

				snooze(300000);
				if (get_thread_info(tid, &info) == B_NO_ERROR) {
					wind = new TStatusWindow();
					wait_for_thread (tid, &result);
					wind->Close();
				}
			}
		}
	}
}

// ---------------------------------------------------------------

void TScriptApp::ReadyToRun()
{
	Quit();
}

// ---------------------------------------------------------------

main()
{
	TScriptApp*		app;

	app = new TScriptApp();
	app->Run();
	delete(app);
	return 0;
}
