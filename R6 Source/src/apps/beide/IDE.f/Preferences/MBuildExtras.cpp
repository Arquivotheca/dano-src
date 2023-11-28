//========================================================================
//	MBuildExtras.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MBuildExtras.h"
#include "MDefaultPrefs.h"
#include "MTextControl.h"
#include "MTextView.h"
#include "IDEMessages.h"
#include "Utils.h"

#include <PopUpMenu.h>
#include <MenuItem.h>
#include <CheckBox.h>
#include <Box.h>
#include <MenuField.h>
#include <Debug.h>

const char * titleBE = "Build Extras";

// ---------------------------------------------------------------------------
//		MBuildExtras
// ---------------------------------------------------------------------------
//	Constructor

MBuildExtras::MBuildExtras(
	MPreferencesWindow&	inWindow,
	BRect				inFrame)
	: MPreferencesView(inWindow, inFrame, "buildextras")
{
	SetPointers(&fOldSettings, &fNewSettings, sizeof(fNewSettings));
}

// ---------------------------------------------------------------------------
//		~MBuildExtras
// ---------------------------------------------------------------------------
//	Destructor

MBuildExtras::~MBuildExtras()
{
}

// ---------------------------------------------------------------------------
//		MessageReceived
// ---------------------------------------------------------------------------

void
MBuildExtras::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgCheckBoxChanged:
		case msgPopupChanged:
		case msgCompileCountChanged:
			ExtractInfo();
			break;

		default:
			MPreferencesView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		ExtractInfo
// ---------------------------------------------------------------------------

void
MBuildExtras::ExtractInfo()
{
	// Compile count
 	BMenuItem* item = fCompileCountPopup->FindMarked();
	ASSERT(item);
	if (item) {
		fNewSettings.concurrentCompiles = fCompileCountPopup->IndexOf(item);
	}
	
	// stop on errors and open error window
	fNewSettings.stopOnError = fStopOnErrorsBox->Value() == 1;
	fNewSettings.openErrorWindow = fOpenErrorWindowBox->Value() == 1;
	
	// Build priority
	item = fBuildPriorityPopup->FindMarked();
	ASSERT(item);
	switch (fBuildPriorityPopup->IndexOf(item)) {
		case 0:
			fNewSettings.compilePriority = B_LOW_PRIORITY;
			break;
		case 1:
			fNewSettings.compilePriority = B_NORMAL_PRIORITY;
			break;
		case 2:
			fNewSettings.compilePriority = B_DISPLAY_PRIORITY;
			break;
		default:
			fNewSettings.compilePriority = B_NORMAL_PRIORITY;
			break;
	}
	
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MBuildExtras::GetData(
	BMessage&	inOutMessage,
	bool		/*isProxy*/)
{
	inOutMessage.AddData(kBuildExtrasPrefs, kMWPrefs, &fNewSettings, sizeof(fNewSettings), false);
}

// ---------------------------------------------------------------------------
//		SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MBuildExtras::SetData(
	BMessage&	inMessage)
{
	ssize_t				length;
	BuildExtrasPrefs*	inPrefs;

	if (B_NO_ERROR == inMessage.FindData(kBuildExtrasPrefs, kMWPrefs, (const void**) &inPrefs, &length))
	{
		ASSERT(length == sizeof(BuildExtrasPrefs));

		fNewSettings = *inPrefs;
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MBuildExtras::UpdateValues()
{
	// Concurrent Compiles popup
	BMenuItem* item = fCompileCountPopup->ItemAt(fNewSettings.concurrentCompiles);
	ASSERT(item);
	if (item && item != fCompileCountPopup->FindMarked()) {
		item->SetMarked(true);
	}
	

	// stop on errors and open error window check boxes
	fStopOnErrorsBox->SetValue(fNewSettings.stopOnError == true ? 1 : 0);
	fOpenErrorWindowBox->SetValue(fNewSettings.openErrorWindow == true ? 1 : 0);

	
	// Build priority
	int32 index = 0;
	switch (fNewSettings.compilePriority) {
		case B_LOW_PRIORITY:
			index = 0;
			break;
		case B_NORMAL_PRIORITY:
			index = 1;
			break;
		case B_DISPLAY_PRIORITY:
			index = 2;
			break;
		default:
			index = 1;
			break;
	}
	item = fBuildPriorityPopup->ItemAt(index);
	if (item && item != fBuildPriorityPopup->FindMarked()) {
		item->SetMarked(true);
	}
}

// ---------------------------------------------------------------------------
//		DoFactorySettings
// ---------------------------------------------------------------------------

void
MBuildExtras::DoFactorySettings()
{
	MDefaultPrefs::SetBuildExtraDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		Title
// ---------------------------------------------------------------------------

const char *
MBuildExtras::Title()
{
	return titleBE;
}

// ---------------------------------------------------------------------------
//		Targets
// ---------------------------------------------------------------------------

TargetT
MBuildExtras::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		AttachedToWindow
// ---------------------------------------------------------------------------

void
MBuildExtras::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BMessage*		msg;
	BPopUpMenu*		popup;
	BMenuField*		menufield;
	float			left = 10.0;
	float			top = 20.0;

	// Box
	r = Bounds();
	box = new BBox(r, "extras");
	box->SetLabel("Build Extras");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Concurrent Compiles popup
	r.Set(left, top, r.right-10, top + 16);

	popup = new BPopUpMenu("compilesmenu");
	fCompileCountPopup = popup;

	msg = new BMessage(msgCompileCountChanged);
	popup->AddItem(new BMenuItem("Same as CPUs", msg));
	for (int32 i = kMinConcurrentCompiles; i <= kMaxConcurrentCompiles; i++)
	{
		char	name[5];
		
		sprintf(name, "%ld", i);

		msg = new BMessage(msgCompileCountChanged);
		popup->AddItem(new BMenuItem(name, msg));
	}

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "compilesbar", "Concurrent Compiles", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(130);
	SetGrey(menufield, kLtGray);
	top += 30.0;
	

	// Build priority popup

	r.Set(left, top, r.right-10, top + 16);
	fBuildPriorityPopup = new BPopUpMenu("prioritymenu");
	fBuildPriorityPopup->AddItem(new BMenuItem("Low", new BMessage(msgPopupChanged)));
	fBuildPriorityPopup->AddItem(new BMenuItem("Normal", new BMessage(msgPopupChanged)));
	fBuildPriorityPopup->AddItem(new BMenuItem("High", new BMessage(msgPopupChanged)));
	fBuildPriorityPopup->SetTargetForItems(this);

	menufield = new BMenuField(r, "prioritybar", "Build Priority", fBuildPriorityPopup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(130);
	SetGrey(menufield, kLtGray);	
	top += 30.0;

	// Stop on errors and error window check box
	
	r.Set(left, top, r.right-10, top + 16.0);
	fStopOnErrorsBox = new BCheckBox(r, NULL, "Stop build on errors", new BMessage(msgCheckBoxChanged)); 
	box->AddChild(fStopOnErrorsBox);
	fStopOnErrorsBox->SetTarget(this);
	SetGrey(fStopOnErrorsBox, kLtGray);
	top += 20.0;

	r.Set(left, top, r.right-10, top + 16.0);
	fOpenErrorWindowBox = new BCheckBox(r, NULL, "Show Errors & Warnings at end of build", new BMessage(msgCheckBoxChanged)); 
	box->AddChild(fOpenErrorWindowBox);
	fOpenErrorWindowBox->SetTarget(this);
	SetGrey(fOpenErrorWindowBox, kLtGray);

}
