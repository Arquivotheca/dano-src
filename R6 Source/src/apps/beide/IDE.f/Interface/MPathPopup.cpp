//========================================================================
//	MPathPopup.cpp
//	Copyright 1997 Metrowerks Corporation, All Rights Reserved.
//========================================================================	

#include "MPathPopup.h"
#include "MIconMenuItem.h"
#include "IDEMessages.h"

#include <Directory.h>
#include <Application.h>
#include <Volume.h>
#include <FSUtils.h>
#include <VolumeRoster.h>

// ---------------------------------------------------------------------------
//		BuildPopup
// ---------------------------------------------------------------------------
//	Build the popup menu.

static void
BuildPathMenu(
	BEntry&	inFile,
	BMenu&	inMenu)
{
	BMessage* msg;
	entry_ref ref;
	status_t err = inFile.GetRef(&ref);
	dev_t device = ref.device;
		
	if (err == B_OK) {
		
		// get the desktop entry for use later
		BVolume	bootVol;
		BVolumeRoster().GetBootVolume(&bootVol);
		BDirectory desktopDir;
		FSGetDeskDir(&desktopDir, bootVol.Device());
		BEntry desktopEntry;
		desktopDir.GetEntry(&desktopEntry);

		BDirectory dir;
		BEntry dirEntry;
		err = inFile.GetParent(&dir);
	
		while (err == B_OK) {
		
			// get the BEntry for the directory,
			// and from that the entry_ref
			err = dir.GetEntry(&dirEntry);
			if (err == B_OK) {
				err = dirEntry.GetRef(&ref);
			}
			
			if (err == B_OK) {
				// Add the menu item to the menu
				msg = new BMessage(B_REFS_RECEIVED);
				msg->AddRef("refs", &ref);

				if (dir.IsRootDirectory()) {
					inMenu.AddItem(new MIconMenuItem(ref, device, msg));
				}
				else {
					inMenu.AddItem(new MIconMenuItem(ref, ref.name, msg));
					if (dirEntry == desktopEntry) {
						// the desktop is always our root (either fake or
						// if we are actually in the desktop directory)
						break;
					}
				}
			}
			
			// get the parent directory and loop
			// If we are at the root, fake the desktop as our parent
			if (dir.IsRootDirectory()) {
				err = dir.SetTo(&desktopEntry);
			}
			else {
				err = dirEntry.GetParent(&dir);
			}
		}
	
		inMenu.SetTargetForItems(&inMenu);
	}
}

// ---------------------------------------------------------------------------
//		OpenItem
// ---------------------------------------------------------------------------

static void
OpenPathItem(
	BMenuItem*	inItem)
{
	BMessage	msg(*inItem->Message());		// copy the message
	
	switch (msg.what)
	{
		case B_REFS_RECEIVED:
		{
			BMessenger		tracker(kTrackerSig);
			tracker.SendMessage(&msg);
		}
			break;
		
		case msgOpenSourceFile:
			be_app_messenger.SendMessage(&msg);
			break;
	}
}

// ---------------------------------------------------------------------------
//		MPathPopup
// ---------------------------------------------------------------------------

MPathPopup::MPathPopup(
	const char * 	inTitle,
	entry_ref&		inRef)
	: BPopUpMenu(inTitle)
{
	BEntry	file(&inRef);
	this->SetFont(be_plain_font);
	BuildPathMenu(file, *this);
}

// ---------------------------------------------------------------------------
//		~MPathPopup
// ---------------------------------------------------------------------------
//	Destructor

MPathPopup::~MPathPopup()
{
}

// ---------------------------------------------------------------------------
//		OpenItem
// ---------------------------------------------------------------------------

void
MPathPopup::OpenItem(
	BMenuItem*	inItem)
{
	OpenPathItem(inItem);
}

// ---------------------------------------------------------------------------
//		MPathPopup
// ---------------------------------------------------------------------------

MPathMenu::MPathMenu(
	const char * 	inTitle,
	entry_ref&		inRef)
	: BMenu(inTitle)
{
	BEntry	file(&inRef);
	this->SetFont(be_plain_font);
	BuildPathMenu(file, *this);
}

// ---------------------------------------------------------------------------
//		~MPathPopup
// ---------------------------------------------------------------------------
//	Destructor

MPathMenu::~MPathMenu()
{
}

// ---------------------------------------------------------------------------
//		OpenItem
// ---------------------------------------------------------------------------

void
MPathMenu::OpenItem(
	BMenuItem*	inItem)
{
	OpenPathItem(inItem);
}

