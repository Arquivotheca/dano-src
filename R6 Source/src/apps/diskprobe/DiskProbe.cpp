//--------------------------------------------------------------------
//	
//	DiskProbe.cpp
//
//	Written by: Robert Polic
//	
//	Copyright 1998 Be, Inc. All Rights Reserved.
//	
//--------------------------------------------------------------------

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <Screen.h>

#include "DiskProbe.h"
#include "SelectWindow.h"
#include "ProbeWindow.h"

BMessage	*print_settings = NULL;


//====================================================================

int main(void)
{	
	TDiskApp	*app;

	app = new TDiskApp();
	app->Run();

	delete app;
	return B_NO_ERROR;
}


//====================================================================

TDiskApp::TDiskApp(void)
		 :BApplication("application/x-vnd.Be-DiskProbe")
{
	BDirectory	dir;
	BEntry		entry;
	BMimeType	mime;
	BPath		path;

	fPrefs.base = B_HEX;
	fPrefs.font_size = (int32) FONT_SIZE;
	fPrefs.case_sensitive = false;

	fSelect = NULL;
	fWindowCount = 0;
	fPanel = NULL;
	fDiskWindow.Set(0, 0, 0, 0);

	find_directory(B_USER_SETTINGS_DIRECTORY, &path, true);
	dir.SetTo(path.Path());
	if (dir.FindEntry("DiskProbe_data", &entry) == B_NO_ERROR) {
		fPrefFile = new BFile(&entry, O_RDWR);
		if (fPrefFile->InitCheck() == B_NO_ERROR) {
			fPrefFile->Read(&fDiskWindow, sizeof(BRect));
			fPrefFile->Read(&fPrefs, sizeof(prefs));
		}
		else {
			delete fPrefFile;
			fPrefFile = NULL;
		}
	}
	else {
		fPrefFile = new BFile();
		if (dir.CreateFile("DiskProbe_data", fPrefFile) != B_NO_ERROR) {
			delete fPrefFile;
			fPrefFile = NULL;
		}
	}
}

//--------------------------------------------------------------------

TDiskApp::~TDiskApp(void)
{
	if (fPanel)
		delete fPanel;
	if (fPrefFile)
		delete fPrefFile;
}

//--------------------------------------------------------------------

void TDiskApp::AboutRequested(void)
{
	(new BAlert("", B_UTF8_ELLIPSIS"by Robert Polic", "Big Deal"))->Go();
}

//--------------------------------------------------------------------

void TDiskApp::ArgvReceived(int32 argc, char **argv)
{
	char			str[256];
	int				i;
	BEntry			entry;
	BPath			path;
	entry_ref		ref;
	TProbeWindow	*window;
  
	for (i = 1; i < argc; i++) {
		entry.SetTo(argv[i]);
		if (entry.InitCheck() == B_NO_ERROR) {
			entry.GetRef(&ref);
			entry.GetPath(&path);
			if ((strstr(path.Path(), "/dev/disk")))
				window = NewWindow(T_DEVICE, (char *)path.Path());
			else
				window = NewWindow(T_FILE, &ref);
			window->Show();
		}
		else {
			sprintf(str, "Sorry, could not open '%s'", argv[i]);
			(new BAlert("", str, "OK", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT))->Go();
		}
	}
}

//--------------------------------------------------------------------

void TDiskApp::MessageReceived(BMessage* msg)
{
	char			*device;
	int32			type;
	BMessenger		*me;
	TProbeWindow	*window;

	switch (msg->what) {
		case M_NEW:
			SelectWindow();
			break;

		case M_OPEN_FILE:
			if (!fPanel) {
				me = new BMessenger(be_app);
				BMessage msg(B_REFS_RECEIVED);
				fPanel = new BFilePanel(B_OPEN_PANEL, me, &fDir, false, true, 
				  &msg);
				delete me;
			}
			else if (fPanel->Window()->Lock()) {
				if (!fPanel->Window()->IsHidden())
					fPanel->Window()->Activate();
				fPanel->Window()->Unlock();
			}
			if (fPanel->Window()->Lock()) {
				if (fPanel->Window()->IsHidden())
					fPanel->Window()->Show();
				fPanel->Window()->Unlock();
			}	
			break;

		case M_OPEN_DEVICE:
			msg->FindString("path", (const char**) &device);
			window = NewWindow(T_DEVICE, device);
			window->Show();
			break;

		case M_WINDOW_CLOSED:
			msg->FindInt32("kind", &type);
			if (type == W_PROBE) {
				fWindowCount--;
				if (!fWindowCount)
					msg->FindRect("frame", &fDiskWindow);
			}
			else {
				fSelect = NULL;
				if (type == W_SELECT_OK)
					break;
			}
			if ((!fWindowCount) && (!fSelect))
				be_app->PostMessage(B_QUIT_REQUESTED);
			break;

		case B_CANCEL:
			if (!fWindowCount) {
				delete fPanel;
				fPanel = NULL;
				be_app->PostMessage(B_QUIT_REQUESTED);
			}
			break;

		case B_REFS_RECEIVED:
			RefsReceived(msg);
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}

//--------------------------------------------------------------------

bool TDiskApp::QuitRequested(void)
{
	if (BApplication::QuitRequested()) {
		if (fPrefFile) {
			fPrefFile->Seek(0, 0);
			fPrefFile->Write(&fDiskWindow, sizeof(BRect));
			fPrefFile->Write(&fPrefs, sizeof(prefs));
		}
		return true;
	}
	else
		return false;
}

//--------------------------------------------------------------------

void TDiskApp::ReadyToRun(void)
{
	if (!fWindowCount)
		SelectWindow();
}

//--------------------------------------------------------------------

void TDiskApp::RefsReceived(BMessage *msg)
{
	int32			item = 0;
	entry_ref		ref;
	TProbeWindow	*window;

	while (msg->HasRef("refs", item)) {
		msg->FindRef("refs", item++, &ref);
		if ((window = FindWindow(ref)))
			window->Activate(true);
		else {
			window = NewWindow(T_FILE, &ref);
			window->Show();
		}
	}
}

//--------------------------------------------------------------------

TProbeWindow* TDiskApp::FindWindow(entry_ref ref)
{
	int32			index = 0;
	TProbeWindow	*window;

	while ((window = (TProbeWindow *)WindowAt(index++))) {
		if (window->Lock()) {
			if ((window->FindView("d_header")) && (window->fRef) && 
				(*(window->fRef) == ref)) {
				window->Unlock();
				return window;
			}
			window->Unlock();
		}
	}
	return NULL;
}

//--------------------------------------------------------------------

TProbeWindow* TDiskApp::NewWindow(int32 type, void *data)
{
	bool			defined_rect = false;
	BRect			r;
	TProbeWindow	*window;
	BScreen			screen(B_MAIN_SCREEN_ID);
	BRect			screen_frame = screen.Frame();

	if ((fDiskWindow.Width()) && (fDiskWindow.Height())) {
		r = fDiskWindow;
		defined_rect = true;
	}
	else
		r.Set(6, TITLE_BAR_HEIGHT,
			  6 + WIND_WIDTH, TITLE_BAR_HEIGHT + WIND_HEIGHT);
	r.OffsetBy(fWindowCount * 20, fWindowCount * 20);
	if ((r.left - 6) < screen_frame.left)
		r.OffsetTo(screen_frame.left + 8, r.top);
	if ((r.left + 20) > screen_frame.right)
		r.OffsetTo(6, r.top);
	if ((r.top - 26) < screen_frame.top)
		r.OffsetTo(r.left, screen_frame.top + 26);
	if ((r.top + 20) > screen_frame.bottom)
		r.OffsetTo(r.left, TITLE_BAR_HEIGHT);
	fWindowCount++;

	window = new TProbeWindow(r, "DiskProbe", data, type, &fPrefs);
	if (!defined_rect) {
		fDiskWindow = window->CalcMaxSize();
		window->Zoom(BPoint(0, 0), 0, 0);
	}
	return window;
}

//--------------------------------------------------------------------

void TDiskApp::SelectWindow(void)
{
	BRect			r;
	BScreen			screen(B_MAIN_SCREEN_ID);
	BRect			screen_frame = screen.Frame();

	if (fSelect)
		fSelect->Activate();
	else {
		r.left = screen_frame.left + ((screen_frame.Width() - SELECT_WIDTH) / 2);
		r.right = r.left + SELECT_WIDTH;
		r.top = screen_frame.top + ((screen_frame.Height() - SELECT_HEIGHT) / 2);
		r.bottom = r.top + SELECT_HEIGHT;
		fSelect = new TSelectWindow(r);
		fSelect->Show();
	}
}
