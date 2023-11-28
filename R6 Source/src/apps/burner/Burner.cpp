//
// Burner.cpp
//
//  The CD burner application.
//
//  by Nathan Schrenk (nschrenk@be.com)

#include <Alert.h>
#include <FilePanel.h>
#include <List.h>
#include <Path.h>
#include <NodeInfo.h>
#include <stdio.h>
#include <TextView.h>
#include <Button.h>
#include <Screen.h>
#include <ScrollView.h>
#include "AudioWrapperDataSource.h"
#include "Burner.h"
#include "BurnerProject.h"
#include "BurnerWindow.h"
#include "CDDriver.h"
#include "MediaFileDataSource.h"
#include "SupportedDrives.h"
#include "TrackListView.h"

// About Window stuff stolen from the Camera app
AboutWnd		*gAboutWnd = NULL;

int main(int /*argc*/, char **/*argv*/)
{
	BApplication *burnerApp = new BurnerApp();
	burnerApp->Run();
}

BurnerApp::BurnerApp()
	: BApplication("application/x-vnd.Be.Burner")
{
	// XXX: check to make sure the projet mime type is in the mime database
	//      and add it if it is not.
	
	fSavePanel = NULL;
	fOpenPanel = NULL;
	fBurnerWindow = NULL;
}

BurnerApp::~BurnerApp()
{
	delete fSavePanel;
	delete fOpenPanel;
}

void BurnerApp::ReadyToRun()
{
	if (CountWindows() == 0) {
		if(!fBurnerWindow) {
			fBurnerWindow = new BurnerWindow("CDBurner");
			fBurnerWindow->Show();
		} else {
			fBurnerWindow->Activate(true);
		}
	}
	PostMessage(kScanForDrives, this);
}


void BurnerApp::AboutRequested()
{
	if (gAboutWnd == NULL)
		gAboutWnd = new AboutWnd();
	else
		gAboutWnd->Activate();
}

void BurnerApp::ArgvReceived(int32 argc, char **argv)
{
	// XXX: do the right thing
	BApplication::ArgvReceived(argc, argv);
}

void BurnerApp::RefsReceived(BMessage *message)
{
	entry_ref ref;

	int32 i = 0;
	while (message->FindRef("refs", i++, &ref) == B_OK) {
		BEntry entry(&ref, true);
		if (entry.InitCheck() != B_OK || !entry.Exists()) {
			// XXX: should this error condition result in an alert being shown?
			continue;
		}
		BNode node(&entry);
		if (node.InitCheck() != B_OK) {
			continue;
		}
		char type[B_MIME_TYPE_LENGTH];
		BNodeInfo nodeInfo(&node);
		nodeInfo.GetType(type);
		
		// look at the type to decide how to load the file
		if (!strcmp(type, BURNER_PROJECT_MIME_TYPE)) {
			BurnerProject *project(NULL);
			bool success = false;
			project = new BurnerProject(&ref);
			// Should error check here...
			if (!fBurnerWindow) {
				fBurnerWindow = new BurnerWindow(B_EMPTY_STRING);
				fBurnerWindow->Show();
			}
			status_t unload = fBurnerWindow->UnloadProject();
			if (unload == B_OK) { 
				fBurnerWindow->SetToProject(project);
				success = true;
			} else if (unload == kMustSave) {
				// XXX: handle saving and then loading of the new project.
				//      some type of new message should be posted back to this
				//      app looper that will save, then unload the project, then
				//      set the new project.x
				fBurnerWindow->Project()->Save();
				fBurnerWindow->UnloadProject();
				fBurnerWindow->SetToProject(project);
			}
			
			if (!success) {
/*				BPath path;
				entry.GetPath(&path);
				BString str("An error occurred while loading the project file '");
				str << path.Path() << "'.\n";
				BAlert *alert = new BAlert("Project Load Failed", str.String(),
					"Ok", NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go(NULL);
*/			}
			// XXX: only handles one project ref right now.
			//      Until multiple window support works, that'll have to do.
			return;
		} else {
			// it wasn't a project, so load it as a track
			bool isData = false;
			int32 insertIndex = -1;
			if (message->HasInt32("insertion_index")) {
				insertIndex = message->FindInt32("insertion_index");
			}
			CDDataSource *src = NULL;
			if (message->FindBool("isData", &isData) == B_OK && isData) {
				src = new CDDataFileDataSource(&entry);
			} else {
				src = new MediaFileDataSource(&entry);
			}
			if (src->InitCheck() == B_OK) {
				if (!isData) {
					AudioWrapperDataSource *wrapper = new AudioWrapperDataSource(src);
					wrapper->SetGainEnabled(false); // gain enabled only during burning
					src = wrapper;
				}
				if(!fBurnerWindow) {
					fBurnerWindow = new BurnerWindow(B_EMPTY_STRING);
					fBurnerWindow->Show();
				}
				fBurnerWindow->Lock();
				TrackListView *listView = dynamic_cast<TrackListView *>(fBurnerWindow->FindView("TrackListView"));
				if (listView != NULL) {
					listView->AddTrack(new CDTrack(src), insertIndex);
				}
				fBurnerWindow->Unlock();
			} else {
				// XXX: the unrecognized files should be grouped into one alert,
				BString str("Burner was unable to load the file '");
				str << ref.name << "'.  Burner supports many different formats"
					<< " of audio files, but it did not recognize this one.\n\n"
					<< " To add a data track, select 'Add Data Track' from\n"
					<< " the 'Disc' menu.\n";
				BAlert *alert = new BAlert("Invalid Audio File", str.String(), "Continue",
					NULL, NULL, B_WIDTH_AS_USUAL, B_STOP_ALERT);
				alert->Go(NULL);
				delete src;
			}
		}
	}
	
}

// returns true if its OK to overwrite the specified file
bool CheckOverwrite(entry_ref *ref)
{
	bool overwrite = false;
	BEntry entry(ref);
	BPath path;
	entry.GetPath(&path);
	
	if (entry.Exists()) {
		// pop up an alert to ask the user whether the file
		// should be overwritten
		BString alertText("The file \"");
		alertText << path.Leaf() << "\" already exists in the specified folder."
			<< "  Do you want to replace it?";
		BAlert *alert = new BAlert(B_EMPTY_STRING, alertText.String(),
			"Cancel", "Replace", NULL, B_WIDTH_AS_USUAL, B_WARNING_ALERT);
		if (alert->Go() == 1) {
			overwrite = true;
		}
	} else {
		// it's OK to "overwrite" a non-existent file.
		overwrite = true;
	}
	
	return overwrite;
}

void BurnerApp::MessageReceived(BMessage *message)
{
	bool checkOverwrite = true;
	bool inSaveRequested = false;
	bool addData = true;

	switch (message->what) {
	// XXX: catch messages here to do things like
	//		* look for a blank CD in the selected CDR drive on a regular basis
	//		* disable everything when a burn is in progress, enable when burn is done
	case kAddAudioTrackMessage:
		addData = false;
		// fall through
	case kAddDataTrackMessage:
		{
			BurnerWindow *window(NULL);
			if (message->FindPointer("window", (void **)&window) == B_OK) {
				DisplayAddTrackWindow(window, addData);
			}
		}
		break;
	case kScanForDrives:
		ScanForDrives();
		break;
	case kOpenProjMessage:
		{
			BurnerWindow *window(NULL);
			BMessage msg(B_REFS_RECEIVED);
			message->FindPointer("window", (void **)&window);
			if (fOpenPanel == NULL) {
				fOpenPanel = new BFilePanel(B_OPEN_PANEL);
			}
			// try to set save panel's folder to a convenient folder
			if (window != NULL) {
				msg.AddPointer("window", window);
				BurnerProject *project = window->Project();
				if (project != NULL) {
					entry_ref ref;
					project->GetEntry(&ref);
					BEntry file(&ref, true);
					if (file.InitCheck() == B_OK) {
						BEntry parent;
						file.GetParent(&parent);
						fOpenPanel->SetPanelDirectory(&parent);
					}
				}
			}
			
			fOpenPanel->SetMessage(&msg);
			fOpenPanel->Show();
		}
		break;
	case B_SAVE_REQUESTED:
		checkOverwrite = false;
		inSaveRequested = true;
		
		// fall through to 'Save Project'
	case kSaveProjMessage:
		{
			BurnerWindow *window(NULL);
			BurnerProject *project(NULL);
			entry_ref ref;
			if (message->FindPointer("window", (void **)&window) == B_OK
				&& window != NULL)
			{
				project = window->Project();
				// pull out ref from msg and stick it into the project
				if (inSaveRequested) {
					char *filename;
					BPath path;
					if (message->FindRef("directory", &ref) == B_OK
						&& message->FindString("name", (const char **)&filename) == B_OK)
					{
						path.SetTo(&ref);
						path.SetTo(path.Path(), filename);
						get_ref_for_path(path.Path(), &ref);
						if (project != NULL) {
							project->SetEntry(&ref);
							BString nuname;
							nuname << "Burner - " << ref.name;
							fBurnerWindow->SetTitle(nuname.String());
						}
					}
				}
			}
			if ((project != NULL) && (project->GetEntry(&ref) == B_OK)) {
				bool doSave = true;
				if (checkOverwrite) {
					doSave = CheckOverwrite(&ref);
				}
				if (doSave && (project->Save() != B_OK)) {
					// XXX: handle write error
					printf("BurnerApp::MessageReceived(): error saving project!\n");
				} else {
					break;
				}
			}
		}
		if (inSaveRequested) {
			printf("BurnerApp::MessageReceived(): bad news... error saving project from B_SAVE_REQUESTED!\n");
			break;
		}
		// fall through to 'Save As' because we have not saved yet
	case kSaveProjAsMessage:
		{
			BurnerWindow *window(NULL);
			message->FindPointer("window", (void **)&window);
			if (fSavePanel == NULL) {
				fSavePanel = new BFilePanel(B_SAVE_PANEL);
			}
			// try to set save panel's folder to a convenient folder
			if (window != NULL) {
				BurnerProject *project = window->Project();
				if (project != NULL) {
					entry_ref ref;
					project->GetEntry(&ref);
					BEntry file(&ref, true);
					if (file.InitCheck() == B_OK) {
						BEntry parent;
						file.GetParent(&parent);
						fSavePanel->SetPanelDirectory(&parent);
					}
				}
			}
			
			BMessage msg(B_SAVE_REQUESTED);
			msg.AddPointer("window", window);
			fSavePanel->SetMessage(&msg);
			fSavePanel->Show();
		}
		break;
	case kCloseRequestedMessage:
		{
			BurnerWindow *win;
			if (message->FindPointer("window", (void **)&win) == B_OK) {
				win->Lock();

				// XXX: check to see if the window has an unsaved project,
				//      or if a burn is in progress

				if (0 /* check here */) {
				
				} else {
					win->SetCanQuit(true);
					win->PostMessage(B_QUIT_REQUESTED, win);
				}
				win->Unlock();				
			}
			
		} break;
	case kWindowClosedMessage:
		{
			BWindow *win;
			if (message->FindPointer("window", (void **)&win) == B_OK) {
				// there is a potential race condition here, because BurnerWindow
				// sends the kWindowClosedMessage before it calls BWindow::Quit().
				// To make sure that the window is gone, we will attempt to
				// lock the window in a loop until the lock fails.  Attempting to
				// lock a dead BLooper is safe, according to Pavel.				
				while (win->Lock()) {
					win->Unlock();
					snooze(10000);
				}
			}
			// quit if all windows are now closed
			// XXX: there needs to be a separate count for main app windows, so that
			// file panel windows, dialogs, etc, don't screw this up.
//			if (CountWindows() == 0) {
				PostMessage(B_QUIT_REQUESTED, this);
//			}
		} break;
	case kQuitMessage:
		{
			BWindow *win;
			int32 i = 0;
			while ((win = WindowAt(i++)) != NULL) {
				win->PostMessage(B_QUIT_REQUESTED, win);
			} 
		}
		break;

	default:
		BApplication::MessageReceived(message);
		break;
	}
}

bool BurnerApp::QuitRequested()
{
	// XXX: do the right thing
	return BApplication::QuitRequested();
}

void BurnerApp::Quit()
{
	// XXX: do the right thing
	BApplication::Quit();
}

void BurnerApp::DisplayAddTrackWindow(BurnerWindow *window, bool data)
{
	BMessage msg(B_REFS_RECEIVED);
	msg.AddPointer("window", window);
	msg.AddBool("isData", data);
	
	if (fOpenPanel == NULL) {
		fOpenPanel = new BFilePanel(B_OPEN_PANEL);
		// try to set save panel's folder to a convenient folder
		BurnerProject *project = window->Project();
		if (project != NULL) {
			entry_ref ref;
			project->GetEntry(&ref);
			BEntry file(&ref, true);
			if (file.InitCheck() == B_OK) {
				BEntry parent;
				file.GetParent(&parent);
				fOpenPanel->SetPanelDirectory(&parent);
			}
		}
	}
	
//	fOpenPanel->SetSaveText(B_EMPTY_STRING);
	fOpenPanel->SetMessage(&msg);
	fOpenPanel->Show();
}


void BurnerApp::ScanForDrives()
{
	DriverInfo *info;
	CDDriver *driver;
	int32 i = 0;
	// remove all drivers that are not in use by a BurnerWindow
	while ((info = (DriverInfo*)fDriverList.ItemAt(i)) != NULL) {
		if (info->owner != NULL) {
			i++;
		} else {
			fDriverList.RemoveItem(i);
			delete info->driver;
			delete info;
		}
	}

	// populate new list by scanning for drivers
	BList newDriverList;
	CDDriver::GetInstances(&newDriverList);

	// add non-conflicting new drivers to main list
	int32 count = fDriverList.CountItems();
	while ((driver = (CDDriver *)newDriverList.RemoveItem((int32)0)) != NULL) {
		// look through drivers left in old list, and don't add a new
		// driver if it is for the same device as an old driver
		bool duplicate = false;
		for (i = 0; i < count; i++) {
			info = (DriverInfo *)fDriverList.ItemAt(i);
			if (!strcmp(info->driver->DeviceName(), driver->DeviceName())) {
				duplicate = true;
				break;
			}
		}
		if (duplicate) {
			delete driver;
		} else {
			fDriverList.AddItem(new DriverInfo(driver));
		}
	}
	
	//notify all windows about device list changes
	BurnerWindow *win;
	count = CountWindows();
	for (i = 0; i < count; i++) {
		if ((win = dynamic_cast<BurnerWindow *>(WindowAt(i))) != NULL) {
			win->PostMessage(kDeviceListChanged, win);
		}
	}
}

int32 BurnerApp::CountDrivers()
{
	return fDriverList.CountItems();
}

CDDriver *BurnerApp::DriverAt(int32 index)
{
	DriverInfo *info = (DriverInfo *)fDriverList.ItemAt(index);
	if (info) {
		return info->driver;
	}
	return NULL;
}

status_t BurnerApp::LockDriver(CDDriver *driver, BurnerWindow *win)
{
	status_t ret = B_ERROR;
	DriverInfo *info;
	int32 count = fDriverList.CountItems();	
	for (int32 i = 0; i < count; i++) {
		info = (DriverInfo *)fDriverList.ItemAt(i);
		if (info->driver == driver && info->owner == NULL) {
			info->owner = win;
			ret = B_OK;
			break;
		}
	}
	
	return ret;
}

status_t BurnerApp::UnlockDriver(CDDriver *driver, BurnerWindow *win)
{
	status_t ret = B_ERROR;
	DriverInfo *info;
	int32 count = fDriverList.CountItems();	
	for (int32 i = 0; i < count; i++) {
		info = (DriverInfo *)fDriverList.ItemAt(i);
		if (info->driver == driver && info->owner == win) {
			info->owner = NULL;
			ret = B_OK;
			break;
		}
	}
		
	return ret;
}



// About Window stuff stolen from the Camera app
AboutWnd::AboutWnd()
	 : BWindow(BRect(0, 0, 400, 300), NULL, B_MODAL_WINDOW_LOOK,
			B_MODAL_APP_WINDOW_FEEL, B_NOT_RESIZABLE | B_ASYNCHRONOUS_CONTROLS)
{
	BRect		r, r2;
	BTextView	*tv;
	BButton		*but;
	BView		*back;

	back = new BView(Bounds(), NULL, B_FOLLOW_ALL, B_WILL_DRAW);
	back->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	r = Bounds();
	r.bottom -= 40;
	r.right -= B_V_SCROLL_BAR_WIDTH;
	r2 = r;
	r2.InsetBy(8, 8);
	tv = new BTextView(r, NULL, r2, B_FOLLOW_ALL, B_WILL_DRAW);
	tv->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	tv->MakeEditable(false);
	tv->MakeSelectable(false);
	tv->SetWordWrap(true);
	tv->SetText(gAboutText);
	back->AddChild(new BScrollView(NULL, tv, B_FOLLOW_ALL, 0, false, true, B_FANCY_BORDER));
	r = Bounds();
	r.bottom -= 8;
	r.top = r.bottom - 24;
	r.left += 160;
	r.right -= 160;
	but = new BButton(r, NULL, "Ok", new BMessage(B_QUIT_REQUESTED));
	back->AddChild(but);
	but->MakeDefault(true);

	AddChild(back);

	// center the window
	BScreen		screen(this);
	BRect		wndR = Frame();
	r = screen.Frame();
	MoveTo((r.left + r.Width() / 2) - wndR.Width() / 2,
		(r.top + r.Height() / 2) - wndR.Height() / 2);
	Show();
}

AboutWnd::~AboutWnd()
{

}

void AboutWnd::Quit()
{
	gAboutWnd = NULL;
	BWindow::Quit();
}
