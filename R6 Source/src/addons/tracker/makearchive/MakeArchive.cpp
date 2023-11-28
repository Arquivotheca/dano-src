/* ++++++++++

   FILE:  make_tar.cpp
   REVS:  $Revision: 1.11 $
   NAME:  steve
   DATE:  Fri Jan 26 16:09:34 PST 1996

   Copyright (c) 1996 by Be Incorporated.  All Rights Reserved.

+++++ */

#include <Directory.h>
#include <Window.h>
#include <Rect.h>
#include <View.h>
#include <FindDirectory.h>
#include <Alert.h>
#include <AppDefs.h>
#include <Message.h>
#include <Errors.h>
#include <Path.h>
#include <image.h>

#include <Debug.h>

#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <TrackerAddon.h>

#define DEFAULT_PARAMS 3

class TStatusView : public BView {
public:
					TStatusView(BRect);
virtual void		Draw(BRect);
};

class TStatusWindow : public BWindow {
public:
					TStatusWindow();
};

void process_refs(entry_ref dir_ref, BMessage* msg, void*)
{

	BAlert*			alert;
	char**			eargv;
	char			name[B_FILE_NAME_LENGTH];
	char			archive_name[B_FILE_NAME_LENGTH];
	TStatusWindow*	wind;
	ulong			type;
	entry_ref		ref;
	int32			result;
	thread_info		info;
	long			count;
	long			i;
	BDirectory		dir;
	BEntry			entry;
	BPath			path;
	thread_id		tid;

	// this add-on requires a selection
	msg->GetInfo("refs", &type, &count);
	if (count == 0) {
		alert = new BAlert("", "You must have a selection to make a tar archive.", "Cancel");
		alert->Go();
		return;
	}

	if (dir.SetTo(&dir_ref) != B_NO_ERROR) {
		alert = new BAlert("", "You must make tar archives from folders.", "Cancel");
		alert->Go();
		return;
	}

	if (dir.GetEntry(&entry) != B_NO_ERROR) {
		alert = new BAlert("", "There was an error accessing the folder while creating a tar archive.", "Cancel");
		alert->Go();
		return;
	}

	entry.GetPath(&path);
	chdir(path.Path());

	// create original name for archive
	strcpy(archive_name, "archive.tar");
	i = 2;
	while (dir.Contains(archive_name))
		sprintf(archive_name, "archive.tar.%d", i++);

	// create argv style buffer to pass to tar tool
	// need string for each entry, default params, and null entry at end
	eargv = (char**)malloc((count + DEFAULT_PARAMS + 1) * (sizeof(char*)));

	for (i = 0; i < count; i++) {
		if (msg->FindRef("refs", i, &ref) == B_NO_ERROR)
			eargv[i + DEFAULT_PARAMS] = strdup(ref.name);
		else {
			alert = new BAlert("", "There was an error finding one of the items you chose to archive.", "Cancel");
			alert->Go();
			count = i;
			goto exit;
		}
	}

	// launch tar tool with argv buffer
	find_directory(B_BEOS_BIN_DIRECTORY, &path);
	path.Append("tar");
	eargv[0] = const_cast<char *>(path.Path());
	eargv[1] = "cf";
	eargv[2] = archive_name;
	eargv[DEFAULT_PARAMS + i] = 0;
	tid = load_image(DEFAULT_PARAMS + i, (const char **)eargv, (const char **)environ);
	if (tid > 0) {
		resume_thread(tid);
	
		// wait a little bit before checking thread status
		snooze(300000);

		// if the thread is still around then bring up status window
		if (get_thread_info(tid, &info) == B_NO_ERROR) {
			wind = new TStatusWindow();
			wait_for_thread(tid, &result);
			wind->Close();
		}
	} else {
		alert = new BAlert("", "Sorry, could not find the tar tool to make an archive.", "OK");
		alert->Go();
	}

exit:
	for (i = 0; i < count; i++)
		free(eargv[i + DEFAULT_PARAMS]);
	free(eargv);
}

// ---------------------------------------------------------------

TStatusView::TStatusView(BRect r)
		    :BView(r, "", B_FOLLOW_NONE, B_WILL_DRAW | B_PULSE_NEEDED)
{
}

// ---------------------------------------------------------------

void TStatusView::Draw(BRect)
{
	SetFont(be_plain_font);
	SetFontSize(9);
	MovePenTo(10, 20);
	DrawString("Creating tar archive...");
}

// ---------------------------------------------------------------


TStatusWindow::TStatusWindow()
			  :BWindow(BRect(100, 120, 300, 160), "Add-on Status", B_TITLED_WINDOW,
					   B_NOT_CLOSABLE | B_NOT_RESIZABLE)
{
	BRect	status_wind_rect(100, 120, 300, 160);

	status_wind_rect.OffsetTo(B_ORIGIN);
	AddChild(new TStatusView(status_wind_rect));
	Show();
}
