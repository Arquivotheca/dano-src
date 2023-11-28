//
// BurnerWindow.cpp
//
//  by Nathan Schrenk (nschrenk@be.com)

#include <Alert.h>
#include <Application.h>
#include <Box.h>
#include <Button.h>
#include <Menu.h>
#include <MenuBar.h>
#include <MenuItem.h>
#include <Roster.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "Burner.h"
#include "BurnerWindow.h"
#include "BurnerProject.h"
#include "BurnControlView.h"
#include "CDDriver.h"
#include "CDPlayerView.h"
#include "TrackListView.h"
#include "TrackEditView.h"

const int32 kToggleColumnMessage	= 'tOGC';

const int32 kRemoveSelTrackMessage	= 'rTRK';
const int32 kProjectLoaded			= 'pLOD';

const char kBurnerTitle[] 			= "Burner";

BurnerWindow::BurnerWindow(const char *title)
	: BWindow(BRect(50, 50, 580, 440), title, B_TITLED_WINDOW, B_ASYNCHRONOUS_CONTROLS)
{
	fCanQuit = false;
	fBurning = false;
	fProject = NULL;
	fProjectLoaded = false;
	
	app_info info;
	if (be_app->GetAppInfo(&info) == B_OK) {
		// position & size window based on last position & size
		if (AutoPosition(&info) != B_OK) {
			// first time run or attrs erased.  position by default.
			
		}
	}
	
	// sign up for track events
	AddTrackListener(this);
	
	// build & add views to window
	BRect bounds = Bounds();

	float mbarHeight;
	float mbarWidth;
	BRect menuRect(Bounds());
	menuRect.bottom = 25;
	fMainMenu = new BMenuBar(menuRect, "Main menu");
	AddChild(fMainMenu);
	fMainMenu->GetPreferredSize(&mbarWidth, &mbarHeight);
	
	fProjectMenu = new BMenu("File");
	fMainMenu->AddItem(fProjectMenu);
	
	BMenuItem *item = new BMenuItem("Open Project", new BMessage(kOpenProjMessage), 'O');
	fProjectMenu->AddItem(item);
	fProjectMenu->AddSeparatorItem();
	item = new BMenuItem("Save Project", new BMessage(kSaveProjMessage), 'S');
	fProjectMenu->AddItem(item);
	item = new BMenuItem("Save Project As", new BMessage(kSaveProjAsMessage), 'S', B_SHIFT_KEY);
	fProjectMenu->AddItem(item);
	fProjectMenu->AddSeparatorItem();
	item = new BMenuItem("About Burner", new BMessage(B_ABOUT_REQUESTED));
	fProjectMenu->AddItem(item);
	item->SetTarget(be_app);
	fProjectMenu->AddSeparatorItem();
	
	item = new BMenuItem("Close", new BMessage(kCloseRequestedMessage));
	fProjectMenu->AddItem(item);
	item = new BMenuItem("Quit", new BMessage(kQuitMessage), 'Q');
	fProjectMenu->AddItem(item);

	fDiscMenu = new BMenu("Disc");
	fMainMenu->AddItem(fDiscMenu);
	item = new BMenuItem("Add Audio Track", new BMessage(kAddAudioTrackMessage));
	fDiscMenu->AddItem(item);
	item = new BMenuItem("Add Data Track", new BMessage(kAddDataTrackMessage));
	fDiscMenu->AddItem(item);
	item = new BMenuItem("Remove Selected Track", new BMessage(kRemoveSelTrackMessage));
	fDiscMenu->AddItem(item);
	
	fSettingsMenu = new BMenu("Settings");
	fMainMenu->AddItem(fSettingsMenu);
	
	fColumnMenu = new BMenu("Columns");
	fSettingsMenu->AddItem(fColumnMenu);

//	fDriveMenu = new BMenu(q);
//	fDriveMenu->SetRadioMode(true);
//	fSettingsMenu->AddItem(fDriveMenu);
	
	BRect rect(bounds);
	rect.top = mbarHeight + 1;
	BView *background = new BView(rect, NULL, B_FOLLOW_ALL, 0);
	background->SetViewUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	background->SetLowUIColor(B_UI_PANEL_BACKGROUND_COLOR);
	AddChild(background);
	
	rect.OffsetTo(0,0);
	rect.InsetBy(5, 5);
	
	// Create column list view...
	BRect rect2(rect);
	rect2.bottom -= 170;
	rect2.right -= 120;
	rect2.top += 5;
	tlView = new TrackListView(rect2);
	background->AddChild(tlView);

	// CDPlayer View
	rect.top = rect2.bottom + 5;
	rect.bottom = rect.top + 42;
	rect.right = background->Bounds().right - 5;
	playerView = new CDPlayerView(rect, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(playerView);

	// Burn view
	rect.top = rect.bottom + 5;
	rect.bottom = background->Bounds().bottom - 5;
	burnView = new BurnControlView(rect, B_FOLLOW_LEFT_RIGHT | B_FOLLOW_BOTTOM);
	background->AddChild(burnView);

//	rect2.bottom = rect2.top + 110;
//	rect2.left = rect2.right + 5;
//	rect2.right = rect.right;
//	
//	playerView = new CDPlayerView(rect2, B_FOLLOW_RIGHT | B_FOLLOW_TOP);
//	background->AddChild(playerView);

//	rect2.top = rect2.bottom + 5;
//	rect2.bottom = rect.top - 5;
	
	// Edit view
	rect2.left = rect2.right + 5;
	rect2.right = background->Bounds().right - 5;
	fEditView = new TrackEditView(rect2, B_FOLLOW_RIGHT | B_FOLLOW_TOP_BOTTOM);
	background->AddChild(fEditView);
	
	SetSizeLimits(bounds.Width(), 800, bounds.Height(), 1000);

	SetToProject(new BurnerProject());
}

void BurnerWindow::MenusBeginning()
{
	// XXX: this needs to be fixed up to not delete and recreate the items each time
	while (BMenuItem *removeItem = fColumnMenu->RemoveItem(0L)) {
		delete removeItem;
	}
	
	BString title;
	BTitledColumn *tColumn;
	int32 columns = tlView->CountColumns();
	for (int32 i = 0; i < columns; i++) {
		tColumn = dynamic_cast<BTitledColumn *>(tlView->ColumnAt(i));
		if (tColumn) {
			BMessage *invokeMsg = new BMessage(kToggleColumnMessage);
			invokeMsg->AddInt32("column", i);
			tColumn->Title(&title);
			BMenuItem *item = new BMenuItem(title.String(), invokeMsg);
			fColumnMenu->AddItem(item);
			if (tColumn->IsVisible()) {
				item->SetMarked(true);
			}
		}
	}
}


BurnerWindow::~BurnerWindow()
{
	delete fProject;
}


void BurnerWindow::MessageReceived(BMessage *msg)
{
	switch (msg->what) {
	case kToggleColumnMessage: {
			int32 colIndex;
			if (msg->FindInt32("column", &colIndex) == B_OK) {
				BColumn *column = tlView->ColumnAt(colIndex);
				column->SetVisible(!column->IsVisible());
			}
		}
		break;		
	case kOpenProjMessage:			// fall through
	case kSaveProjMessage:			// fall through
	case kSaveProjAsMessage:		// fall through
	case kCloseRequestedMessage:	// fall through
	case kQuitMessage:
	case kAddAudioTrackMessage:
	case kAddDataTrackMessage:
		// punt message to the app
		msg->AddPointer("window", this);
		be_app->PostMessage(msg, be_app);
		break;
	case kDeviceListChanged:
		RebuildDeviceMenu();
		break;
	case kProjectLoaded:
		fProjectLoaded = true;
		break;
	case TRACK_ADDED:		// fall through
	case TRACK_DELETED:		// fall through
	case TRACK_MOVED:
		if (fProject && fProjectLoaded) {
			fProject->SetDirty(true);
			fProject->SetTrackList(tlView->GetTrackList());
		}
		break;
	case kRemoveSelTrackMessage:
		if (!BurnInProgress()) {
			tlView->RemoveSelectedTracks();
		}
		break;
	case kBurnStarting:
		fBurning = true;
		// XXX: notify TrackEditView, CDPlayerView, TrackListView
		// XXX: disable open project menu item
//		printf("BurnerWindow::MessageReceived(): kBurnStarting not implemented\n");
		fEditView->SetEnabled(false);
		tlView->SetEditEnabled(false);
		burnView->GetBurnButton()->SetEnabled(true); // re-enable button, now is abort button
		burnView->SetStatusMessage("Burning in progress...", true);
		break;
	case kBurnEnding:
		fBurning = false;
		// XXX: notify TrackEditView, CDPlayerView, TrackListView
//		printf("BurnerWindow::MessageReceived(): kBurnEnding not implemented\n");
		fEditView->SetEnabled(true);	
		tlView->SetEditEnabled(true);
		 // re-enable button if a burn can happen again (unlikely?)
		burnView->GetBurnButton()->SetEnabled(burnView->CanBurn());
		break;
	default:
		BWindow::MessageReceived(msg);
		break;	
	}
}


void BurnerWindow::Quit()
{
	status_t unload = UnloadProject();
	if (unload == B_OK) { 
		BMessage msg(kWindowClosedMessage);
		msg.AddPointer("window", this);
		be_app->PostMessage(&msg, be_app);
		BWindow::Quit();
	} else if (unload == kMustSave) {
		// XXX: must save and then quit
		BMessage saveMsg(kSaveProjMessage);
		saveMsg.AddBool("quit_after_save", true);
		PostMessage(&saveMsg, this);
	}
}


bool BurnerWindow::QuitRequested()
{
	if (CanQuit()) {
		return true;
	} else {
		BMessage msg(kCloseRequestedMessage);
		msg.AddPointer("window", this);
		be_app->PostMessage(&msg, be_app);
		return false;
	}
}


status_t BurnerWindow::AutoPosition(app_info */*info*/)
{
	// XXX: implement this for real
	
	// read last position & size from attributes

	// check to make sure this position and size are valid for
	// this screen
		
	// move and resize the window
	return B_OK;
}

void BurnerWindow::AddTrackListener(BHandler *handler)
{
	fListenerList.AddItem(handler);
}

status_t BurnerWindow::RemoveTrackListener(BHandler *handler)
{
	bool removed = fListenerList.RemoveItem(handler);
	return (removed) ? B_OK : B_ERROR;
}

void BurnerWindow::SendTrackMessage(BMessage *msg)
{
	BHandler *handler;
	BLooper *looper;
	int32 i = 0;
	while ((handler = static_cast<BHandler *>(fListenerList.ItemAt(i++))) != NULL) {
		looper = handler->Looper();
		if (looper) {
			looper->PostMessage(msg, handler);
		}
	}
}

status_t BurnerWindow::SetToProject(BurnerProject *project)
{
	// XXX: should modify this so that this method locks the window
	//      and properly unloads things, and then loads tracks one
	//      at a time in such a way that the display updates nicely
	status_t ret = B_ERROR;

	BString title(kBurnerTitle);
	title << " - " << project->Name();
	if (Lock()) {
		SetTitle(title.String());
		fProjectLoaded = false;
		fProject = project;
		CDTrack *track = project->TrackList();
		CDTrack *next;
		while (track != NULL) {
			// when TrackListView::AddTrack() gets called, it will wipe out
			// the Next() pointer, so save it before calling AddTrack()
			next = track->Next();
			tlView->AddTrack(track);
			// allow window to update
			Unlock();
			Lock();
			track = next;
		}
		// ^^^ <HACK> Just calling this function *shouldn't* cause the project to be
		// ^^^ dirty. The AddTrack() above blindly sets the dirty flag which is wrong,
		// ^^^ since we aren't really adding a track but rather _loading_ a track. </HACK>
		// ^^^ fProject->SetDirty(false);
		fProject->SetOwnsTracks(false);
		PostMessage(kProjectLoaded, this);
		Unlock();
	}
	return ret;
}

status_t BurnerWindow::UnloadProject()
{
	status_t ret = B_ERROR;
	if (Lock()) {
		// check if old project unsaved, and ask user what to do
		if (fProject != NULL) {
			if (fProject->InitCheck() == B_OK && fProject->IsDirty()) {
				BString alertTxt("The current project has changed and has not been saved.");
				alertTxt << "  Do you wish to save it now?";
				BAlert *saveAlert = new BAlert(B_EMPTY_STRING, alertTxt.String(),
					"Discard", "Save", "Cancel", B_WIDTH_AS_USUAL, B_WARNING_ALERT);
				Unlock();
				int32 choice = saveAlert->Go();
				if (choice == 2) { // cancel
					return B_ERROR;
				} else if (choice == 1) { // save
					return kMustSave; 
				} else {
					// must re-lock the window
					Lock();
				}
			}
			// either the project was not dirty or the user chose "Discard"
			delete fProject;
			fProject = NULL;
		}
		tlView->RemoveAllTracks();
		SetTitle(kBurnerTitle);
		ret = B_OK;
		Unlock();
	}
	return ret;
}

BMenuBar *BurnerWindow::MainMenu()
{
	return fMainMenu;
}

BurnerProject *BurnerWindow::Project()
{
	return fProject;
}


bool BurnerWindow::BurnInProgress()
{
	return fBurning;
}

bool BurnerWindow::CanQuit()
{
	return fCanQuit && !fBurning;
}

void BurnerWindow::SetCanQuit(bool okToQuit)
{
	fCanQuit = okToQuit;
}

CDDriver *BurnerWindow::GetSelectedDriver()
{
	BMenu *deviceMenu = burnView->GetDeviceMenu();
	CDRMenuItem *item = dynamic_cast<CDRMenuItem *>(deviceMenu->FindMarked());
	if (item) {
		return item->Driver();
	}
	return NULL;
}

void BurnerWindow::RebuildDeviceMenu()
{
	BurnerApp *app = (BurnerApp *)be_app;
	BMenuItem *item;
	BMenu *deviceMenu = burnView->GetDeviceMenu();
	// remove all items from the drive menu
	while ((item = deviceMenu->RemoveItem((int32)0)) != NULL) {
		delete item;
	}
	int32 i = 0;
	CDDriver *driver;
	CDRMenuItem *cdItem;
	// add drivers from latest scan into the drive menu
	while ((driver = app->DriverAt(i++)) != NULL) {
		cdItem = new CDRMenuItem(driver, new BMessage(kDeviceSelectMessage));
		cdItem->SetTarget(burnView);
		deviceMenu->AddItem(cdItem);
	}
	if ((item = deviceMenu->ItemAt(0)) != NULL) {
			item->SetMarked(true);
			((BInvoker *)item)->Invoke();
	} else {
		item = new BMenuItem("<No compatible CDR devices found>", NULL);
		deviceMenu->AddItem(item);
		item->SetMarked(true);
		item->SetEnabled(false);
	}
}


//-------------------------------------------------------------------------------


CDRMenuItem::CDRMenuItem(CDDriver *driver, BMessage *msg)
	: BMenuItem(driver->DeviceName(), msg),
	fDriver(driver)
{
}

CDRMenuItem::~CDRMenuItem()
{
//	delete fDriver;
}

CDDriver *CDRMenuItem::Driver()
{
	return fDriver;
}

