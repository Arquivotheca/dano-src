//--------------------------------------------------------------------
//	
//	Workspace.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1995 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#define DEBUG 1
#include <Debug.h>
#include <PopUpMenu.h>
#include <stdio.h>
#ifndef _OS_H
#include <OS.h>
#endif
#ifndef WORKSPACE_H
#include "Workspace.h"
#endif
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>
#include <Screen.h>

void	get_counts(long*, long*);

//====================================================================

int main()
{	
	TWSApp().Run();
	return B_NO_ERROR;
}

//--------------------------------------------------------------------

TWSApp::TWSApp()
		  :BApplication("application/x-vnd.Be-WORK")
{
	arg_only = false;
}

//--------------------------------------------------------------------

void TWSApp::AboutRequested()
{
	(new BAlert("", "...by Robert Polic", "Big Deal"))->Go();
}


//--------------------------------------------------------------------

void TWSApp::ArgvReceived(int32 argc, char **argv)
{
	int32	ws;

	if (argc > 1) {
		ws = strtol(argv[1], 0, 0);
		if ((ws >= 0) && (ws < count_workspaces()))
			activate_workspace(ws);
		arg_only = TRUE;
	}	
}

//--------------------------------------------------------------------

void TWSApp::MessageReceived(BMessage* msg)
{
	char		string[256];
	BAlert		*myAlert;

	switch (msg->what) {
		case MENU_GET_COUNT:
			sprintf(string, "Current number of workspaces: %ld",
					count_workspaces());
			myAlert = new BAlert("", string, "Thanks!");
			myAlert->Go();
			break;

		case MENU_SET_COUNT:
#if 0
			long count;
			short loop;
			char data[256];
			int	ref;
			mkdir("/boot/system/settings", 0777);
			if ((ref = open("/boot/system/settings/Desktop_settings", 0)) < 0) {
				if ((ref = creat("/boot/system/settings/Desktop_settings", 0777)) >= 0) {
					for (loop = 0; loop < 4; loop++)
						write(ref, &data, sizeof(data));
				}
			}
			if (ref >= 0)
				close(ref);
			count = (msg->FindInt32("index") + 1);
			set_workspace_count(count);
			if (count_workspaces() != count) {
				BMenu *subMenu;
				subMenu = fMenu->SubmenuAt(1);
				if (subMenu) {
					BMenuItem *item = subMenu->ItemAt(count_workspaces() - 1);
					item->SetMarked(TRUE);
				}
			}
			if (count_workspaces() > count) {
				sprintf(string, "You can't reduce the number of workspaces to \
%d because there are windows that would be removed. Close those windows first.",
				count);
				myAlert = new BAlert("", string, "OK");
				myAlert->Go();
			}
#endif
			break;

		case MENU_QUIT:
			fWSWindow->Quit();
			PostMessage(B_QUIT_REQUESTED);
			break;
	}
}

//--------------------------------------------------------------------

void TWSApp::ReadyToRun(void)
{
	char			string[256];
	long			loop;
	long			ws;
	BDirectory		dir;
	BEntry			entry;
	BFile			file;
	BMenu			*subMenu;
	BMenuItem		*item;
	BPath			path;
	BRect			r;

	if (arg_only)
		be_app->PostMessage(B_QUIT_REQUESTED);
	else {
		fMenu = new BPopUpMenu("Workspace", false, false);
		item = new BMenuItem("About Workspaces...", new BMessage(B_ABOUT_REQUESTED));
		fMenu->AddItem(item);

		subMenu = new BMenu("Workspaces");
		subMenu->SetRadioMode(TRUE);
		ws = count_workspaces();
		for (loop = 1; loop <= 32; loop++) {
			sprintf(string, "%2ld", loop);
			item = new BMenuItem(string, new BMessage(MENU_SET_COUNT));
			subMenu->AddItem(item);
			if (ws == loop)
				item->SetMarked(TRUE);
		}
		fMenu->AddItem(new BMenuItem(subMenu));

		//item = new BMenuItem("Workspace count...", new BMessage(MENU_GET_COUNT));
		//fMenu->AddItem(item);

		fMenu->AddSeparatorItem();

		item = new BMenuItem("Quit Workspaces", new BMessage(MENU_QUIT), 'Q');
		fMenu->AddItem(item);

#if 0
		SetMainMenu(fMenu);
#endif

		r.Set(-2, -2, -1, -1);
		find_directory(B_USER_SETTINGS_DIRECTORY, &path);
		dir.SetTo(path.Path());
		if (dir.FindEntry("Workspace_data", &entry) == B_NO_ERROR) {
			file.SetTo(&entry, O_RDONLY);
			if (file.InitCheck() == B_NO_ERROR) {
				file.Read(&r, sizeof(BRect));
			}
		}
	
		BScreen screen( B_MAIN_SCREEN_ID );
		BRect screenframe = screen.Frame();

		if (!screenframe.Contains(r.LeftTop()))
			r.Set(screenframe.right - 168, screenframe.bottom - 129,
				  screenframe.right - 8, screenframe.bottom - 8);

		fWSWindow = new TWSWindow(r, "Workspaces");
	}
}


//====================================================================

const uint32 _WORKSPACE_WINDOW_FLAG_ = 0x00008000;

TWSWindow::TWSWindow(BRect rect, char *title)
	          :BWindow(rect, title, B_TITLED_WINDOW, B_AVOID_FRONT |
													 _WORKSPACE_WINDOW_FLAG_,
													 B_ALL_WORKSPACES)
{
	fBounds = rect;
	SetSizeLimits(44.0, 10000.0, 47.0, 10000.0);
	Show();
}

//--------------------------------------------------------------------

void TWSWindow::MessageReceived(BMessage* msg)
{
	if (msg->WasDropped()) {
		BPoint loc = msg->DropPoint();
		HandleMessageDropped(msg, loc);
	} else {
		BWindow::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TWSWindow::HandleMessageDropped(BMessage *msg, BPoint loc)
{
	bool		result = false;
	long		h, v;
	long		count;
	long		space;
	long		i;
	ulong		type;
	BAlert		*alert;
	BMessage	*Bopn;
	BMessenger	*tracker;
	BRect		bounds;
	entry_ref	ref;

//+	PRINT(("what=%x (%.4s)\n", msg->what, (char*) &(msg->what)));

	if (msg->FindRef("refs", &ref) == B_NO_ERROR) {
		get_counts(&h, &v);
		bounds = Bounds();
		space = ((int)loc.y / ((int)bounds.bottom / v)) * h +
				((int)loc.x / ((int)bounds.right / h));
		tracker = new BMessenger("application/x-vnd.BE-TRAK", -1, NULL);
//+		PRINT(("ref.name=%s\n", ref.name));
		if (!tracker->IsValid()) {
			delete tracker;
			alert = new BAlert("", "The drop opening feature requires the Tracker.",
								"Sorry");
			alert->Go();
		}
		else {
			msg->GetInfo("refs", &type, &count);
			for (i = 0; i < count; i++) {
//+				Bopn = new BMessage('Bopn');
				Bopn = new BMessage(B_REFS_RECEIVED);
				msg->FindRef("refs", i, &ref);
				Bopn->AddRef("refs", &ref);
				Bopn->AddInt32("ws", space);
				tracker->SendMessage(Bopn);
				delete Bopn;
			}
			result = true;
		}
	}
	return result;
}

//--------------------------------------------------------------------

bool TWSWindow::QuitRequested()
{
	BDirectory		dir;
	BEntry			entry;
	BFile			file;
	BPath			path;
	BRect			r;

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry("Workspace_data", &entry) == B_NO_ERROR)
		file.SetTo(&entry, O_RDWR);
	else
		dir.CreateFile("Workspace_data", &file);

	if (file.InitCheck() == B_NO_ERROR) {
		r = Frame();
		file.Write(&r, sizeof(BRect));
	}

	be_app->PostMessage(B_QUIT_REQUESTED);
	return true;
}

//--------------------------------------------------------------------

void TWSWindow::ScreenChanged(BRect s, color_space)
{
	BRect r(Frame());
	r.top += 20;
	r.left += 20;
	if (!s.Contains(r.LeftTop()))
		Zoom(s.LeftTop(), 0.0, 0.0);
}

//--------------------------------------------------------------------

void TWSWindow::WorkspaceActivated(long, bool)
{
	BRect r(Frame());
	r.top += 20;
	r.left += 20;
	if (!BScreen(this).Frame().Contains(r.LeftTop()))
		Zoom(r.LeftTop(), 0.0, 0.0);
}

//--------------------------------------------------------------------

void TWSWindow::Zoom(BPoint , float rec_width, float rec_height)
{
	bool			moveit = true;
	BPoint			pos;
	BRect			bounds;

	BScreen screen( this );
	BRect screenframe(screen.Frame());

	bounds = Frame();
	if ((bounds.right != screenframe.right - 8) ||
		(bounds.bottom != screenframe.bottom - 8)) {
		fBounds = bounds;
		pos.x = (screenframe.right - 8) - (bounds.right - bounds.left);
		pos.y = (screenframe.bottom - 8) - (bounds.bottom - bounds.top);
	} else {
		if ((rec_width) || (rec_height)) {
			pos.x = fBounds.left;
			pos.y = fBounds.top;
			if (!screenframe.Contains(pos))
				moveit = false;
		} else
			moveit = false;
	}
	if (moveit)
		MoveTo(pos);
}

/*---------------------------------------------------------------*/
/* Return an h and v pair for the defined number of workspaces.	 */
/*---------------------------------------------------------------*/

void get_counts(long *h, long *v)
{
	switch (count_workspaces()) {
		case 1:
			*h = 1; *v = 1; break;
		case 2:
			*h = 2; *v = 1; break;
		case 3:
			*h = 3; *v = 1; break;
		case 4:
			*h = 2; *v = 2; break;
		case 5:
			*h = 5; *v = 1; break;
		case 6:
			*h = 3; *v = 2; break;
		case 7:
			*h = 7; *v = 1; break;
		case 8:
			*h = 4; *v = 2; break;
		case 9:
			*h = 3; *v = 3; break;
		case 10:
			*h = 5; *v = 2; break;
		case 11:
			*h = 11; *v = 1; break;
		case 12:
			*h = 4; *v = 3; break;
		case 13:
			*h = 13; *v = 1; break;
		case 14:
			*h = 7; *v = 2; break;
		case 15:
			*h = 5; *v = 3; break;
		case 16:
			*h = 4; *v = 4; break;
		case 17:
			*h = 17; *v = 1; break;
		case 18:
			*h = 6; *v = 3; break;
		case 19:
			*h = 19; *v = 1; break;
		case 20:
			*h = 5; *v = 4; break;
		case 21:
			*h = 7; *v = 3; break;
		case 22:
			*h = 11; *v = 2; break;
		case 23:
			*h = 23; *v = 1; break;
		case 24:
			*h = 6; *v = 4; break;
		case 25:
			*h = 5; *v = 5; break;
		case 26:
			*h = 13; *v = 2; break;
		case 27:
			*h = 9; *v = 3; break;
		case 28:
			*h = 7; *v = 4; break;
		case 29:
			*h = 29; *v = 1; break;
		case 30:
			*h = 6; *v = 5; break;
		case 31:
			*h = 31; *v = 1; break;
		case 32:
			*h = 8; *v = 4; break;
	}
}
