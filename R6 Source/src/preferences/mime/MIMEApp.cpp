//
// (c) 1997, Be Inc.
//

// ToDo list:
//

#ifndef _BE_H
#include <Alert.h>
#include <Directory.h>
#include <File.h>
#include <FindDirectory.h>
#include <Path.h>
#include <stdio.h>
#include <string.h>
#include <Screen.h>
#endif

#include <Debug.h>

#include "MIMEApp.h"
#include "MIMEPanel.h"
#include "MIMEAttributes.h"
#include "IconWindow.h"
#include "AppMimeWindow.h"
#include "FileTypeWindow.h"

void
MIMEApp::GetInitialSettings(BRect *rect, bool *showAllTypes)
{

	const char *prefsName = "MIMEPrefs_settings";
	
	BPoint winPos(rect->LeftTop());

	BPath directoryPath;
	if (find_directory(B_USER_SETTINGS_DIRECTORY, &directoryPath, true) == B_OK) {
		BPath path(directoryPath);
		path.Append (prefsName);
		prefs = new BFile(path.Path(), O_RDWR);
		
		if (prefs->InitCheck() != B_NO_ERROR) {
			PRINT(("did not find prefs at %s, creating new.\n", path.Path()));
			// no prefs file yet, create a new one
			BDirectory dir(directoryPath.Path());
			if (dir.InitCheck() == B_NO_ERROR)
				dir.CreateFile(prefsName, prefs);
		}
		
		if (prefs->InitCheck() == B_NO_ERROR) {
			if (prefs->Read(&winPos, sizeof(BPoint)) > 0
				&& BScreen(B_MAIN_SCREEN_ID).Frame().Contains(winPos))
				rect->OffsetTo(winPos);
			bool showAllTmp;
			if (prefs->Read(&showAllTmp, sizeof(bool)) > 0)
				*showAllTypes = showAllTmp;
		}
		else {
			delete prefs;
			prefs = NULL;
			PRINT(("creating prefs failed\n"));
		}
	}
}

void 
MIMEApp::SaveInitialSettings(BRect rect, bool showAllTypes)
{
	if (!prefs)
		return;

	BPoint winPos(rect.LeftTop());
	
	// write out the window position	
	prefs->Seek(0, 0);
	prefs->Write(&winPos, sizeof(BPoint));
	prefs->Write(&showAllTypes, sizeof(bool));
}

MIMEApp::MIMEApp(const char *signature)
	:	BApplication(signature),
		savePanel(0),
		openPanel(0),
		prefs(0)
{
}

MIMEApp::~MIMEApp()
{
	delete prefs;
	delete savePanel;
	delete openPanel;
}

void 
MIMEApp::MessageReceived(BMessage *msg)
{
	BMessenger self(this);
	BFilePanel *panel;
	
	entry_ref ref;
	switch (msg->what) {
		case M_NEW_RESOURCE:
			new AppMimeTypeWindow(BPoint(100, 100), NULL);
			break;

		case M_OPEN_WINDOW:
			panel = OpenPanel();
			panel->SetTarget(self);
			panel->Window()->SetTitle("Open:");
			panel->Show();
			break;
		case B_SIMPLE_DATA:
			if (msg->HasRef("refs")) {
				entry_ref ref;
				OpenWindow(msg);
			}
			break;
		case B_REFS_RECEIVED:
			PRINT(("dispatching refs received \n"));
			RefsReceived(msg);
			break;

		default:
			BApplication::MessageReceived(msg);
	}
}

void 
MIMEApp::OpenWindow(BMessage *msg)
{
	int32 count = 0;
	uint32 type = 0;
	msg->GetInfo("refs", &type, &count);
	if (count == 1) {
		entry_ref ref;
		msg->FindRef("refs", &ref);
		OpenWindow(&ref);
	} else {
		BList *refList = new BList(count);
		for (long i = 0; i < count; i++) {
			entry_ref *ref = new entry_ref;
			if (msg->FindRef("refs", i, ref) == B_NO_ERROR) 
				refList->AddItem(ref);
			else {
				delete ref;
				break;
			}
		}

		OpenWindow(refList);	
	}
}

void
MIMEApp::OpenWindow(BList *refList)
{
	//
	//	mimeset all files
	//
	int32 numItems = refList->CountItems();
	for (int32 item = 0; item < numItems; ++item) {
		BEntry entry((entry_ref*) refList->ItemAt(item), true);
		BPath path;
		if (entry.GetPath(&path) == B_OK) 
			update_mime_info(path.Path(), false, true, false);
	}	

	new FileTypeWindow(BPoint(100, 100), refList);
}

void
MIMEApp::OpenWindow(entry_ref *ref)
{
	//
	// 	If we already have this file open,just pull up the window
	//
	for (int32 index = CountWindows() - 1; index >= 0; index--) {
		MIMETypeWindow *window = dynamic_cast<MIMETypeWindow *>
			(WindowAt(index));
		
		if (window && window->Ref() && *(window->Ref()) == *ref) {
			PRINT(("request to open %s, already open %s\n", 
				ref->name, window->Ref()->name));
			window->Activate();
			return;
		}
	}

	//
	//	Mimeset file	
	//
	BEntry entry(ref, true);
	BPath path;
	if (entry.GetPath(&path) == B_OK) 
		update_mime_info(path.Path(), false, true, false);


	//
	//	Open file
	//
	char buffer[255];
	BFile file(ref, O_RDWR);
	status_t result = file.InitCheck();
	if (result != B_NO_ERROR) {
		sprintf(buffer, "Could not open file %s, %s", ref->name,
			strerror(result));
		(new BAlert("", buffer, "OK"))->Go();
		return;
	}

	// decide between an app and a simple type here
	// do smart window positioning here
	BNodeInfo nodeInfo(&file);
	if (nodeInfo.InitCheck() != B_NO_ERROR) {
		PRINT(("error opening nodeinfo for file\n"));
		return;
	}
	nodeInfo.GetType(buffer);
	
	if (strcmp(buffer, B_APP_MIME_TYPE) == 0
		|| strcmp(buffer, B_RESOURCE_MIME_TYPE) == 0)
		new AppMimeTypeWindow(BPoint(100, 100), ref);
	else {
		BList *refList = new BList(1);
		refList->AddItem(new entry_ref(*ref));
		new FileTypeWindow(BPoint(100, 100), refList);
	}
}

void
MIMEApp::RefsReceived(BMessage *message)
{
	int32 count = 0;
	uint32 type = 0;
	entry_ref ref;

	PRINT(("refs received \n"));
	OpenWindow(message);
}

void
MIMEApp::ArgvReceived(int32 argc, char **argv)
{
	if (argc == 2) {
		entry_ref ref;
		if (get_ref_for_path(argv[1], &ref) == B_NO_ERROR)
			OpenWindow(&ref);
	} else if (argc > 2) {
		BList *refList = new BList(argc - 1);
		for (int32 i = 1; i < argc; i++) {
			entry_ref *ref = new entry_ref;
			if (get_ref_for_path(argv[i], ref) == B_NO_ERROR)
				refList->AddItem(ref);
		}

		OpenWindow(refList);
	}
}

BFilePanel *
MIMEApp::SavePanel()
{
	if (!savePanel)
		savePanel = new BFilePanel(B_SAVE_PANEL);

	return savePanel;
}

BFilePanel *
MIMEApp::OpenPanel()
{
	if (!openPanel)
		openPanel = new BFilePanel(B_OPEN_PANEL);

	return openPanel;
}

bool 
MIMEApp::QuitRequested()
{
	for (int32 index = CountWindows() - 1; index >=0 ; index--) {
		BWindow *window = WindowAt(index);
		// first get rid of regular app or file editors
		// ignore icon editors and open/save panels
		if (window
			&& !dynamic_cast<TIconWindow *>(window)
				// icon views have to get quit by their owning panels
			&& (!savePanel || window != savePanel->Window())
			&& (!openPanel || window != openPanel->Window())) {
				// open/save panels will get deleted later

			if (window->QuitRequested()) {
				window->Lock();
				window->Quit();
			} else {
				PRINT(("window %s refusing to quit\n", window->Title()));
				return false;
			}
		}
	}
#if DEBUG
	for (int32 index = CountWindows(); index >=0 ; index--) {
		BWindow *window = WindowAt(index);		
		ASSERT(!dynamic_cast<TIconWindow *>(window));
	}
#endif

	return BApplication::QuitRequested();
}
