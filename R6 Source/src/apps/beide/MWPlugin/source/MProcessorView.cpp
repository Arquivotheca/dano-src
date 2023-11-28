//========================================================================
//	MProcessorView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MProcessorView.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "Utils.h"
#include "IDEMessages.h"
#include "CString.h"
#include "MPlugin.h"

#include <Menu.h>
#include <MenuItem.h>
#include <MenuField.h>
#include <Box.h>
#include <CheckBox.h>
#include <PopUpMenu.h>
#include <Debug.h>
#include <Window.h>

const char kNoOptText[] = "No optimizations.";
const char kOptOneText[] = "Dead code elimination and global register allocation.";
const char kOptTwoText[] = "Common subexpression elimination, copy propagation, dead code elimination, "
"and global register allocation.";
const char kOptThreeText[] = "Common subexpression elimination, loop-invariant code motion, strength reduction, "
"copy propagation, loop transformations, dead code elimination, and global register allocation.";
const char kOptFourText[] = "Repeated common subexpression elimination and loop-invariant code motion, "
"strength reduction, copy propagation, loop transformations, dead code elimination, and global register allocation.";

const char * titleProc = "Code Generation/PPC Processor";

// ---------------------------------------------------------------------------
//		 MProcessorView
// ---------------------------------------------------------------------------
//	Constructor

MProcessorView::MProcessorView(
	BRect				inFrame)
	: MPlugInPrefsView(inFrame, "warningsview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		 ~MProcessorView
// ---------------------------------------------------------------------------
//	Destructor

MProcessorView::~MProcessorView()
{
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MProcessorView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgCheckBoxChanged:
			UpdateCheckBoxes();
			break;

		case msgPopupChanged:
			UpdatePopups();
			break;

		default:
			MPlugInPrefsView::MessageReceived(inMessage);
			break;
	}
}

// ---------------------------------------------------------------------------
//		 UpdateCheckBoxes
// ---------------------------------------------------------------------------

void
MProcessorView::UpdateCheckBoxes()
{
	fNewSettings.pStringsROnly = fStringsROnly->Value() == 1;
	fNewSettings.pStaticInTOC = fStaticInTOC->Value() == 1;
	fNewSettings.pFPContractions = fFPContractions->Value() == 1;
	fNewSettings.pEmitTraceback = fTraceback->Value() == 1;
	fNewSettings.pPeepholeOn = fPeephole->Value() == 1;
	fNewSettings.pEmitProfile = fProfile->Value() == 1;

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 UpdatePopups
// ---------------------------------------------------------------------------

void
MProcessorView::UpdatePopups()
{
	BMenuItem*		item;
	
	// Instruction Scheduling
	item = fInstructionScheduling->FindMarked();
	ASSERT(item);
	if (item)
		fNewSettings.pInstructionScheduling = (SchedulingT) fInstructionScheduling->IndexOf(item);

	// Speed/Size
	item = fSpeedSizePopup->FindMarked();
	ASSERT(item);
	if (item)
		fNewSettings.pOptimizeForSpeed = fSpeedSizePopup->IndexOf(item) == 0;

	// Global Optimization
	item = fOptimizationLevelPopup->FindMarked();
	ASSERT(item);
	if (item)
	{
		fNewSettings.pOptimizationLevel = fOptimizationLevelPopup->IndexOf(item);
		SetInfoText();
	}

	ValueChanged();
}


// ---------------------------------------------------------------------------
//		 GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MProcessorView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();
	inOutMessage.AddData(kProcessorPrefs, kMWCCPlugType, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		 SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MProcessorView::SetData(
	BMessage&	inMessage)
{
	ProcessorPrefs*		prefs;
	long				length;

	if (B_NO_ERROR == inMessage.FindData(kProcessorPrefs, kMWCCPlugType, (const void**) &prefs, &length))
	{
		ASSERT(length == sizeof(ProcessorPrefs));

		fNewSettings = *prefs;
		fNewSettings.SwapBigToHost();
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		 UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MProcessorView::UpdateValues()
{
	BMenuItem*		item;

	// CheckBoxes
	fStringsROnly->SetValue(fNewSettings.pStringsROnly);
	fStaticInTOC->SetValue(fNewSettings.pStaticInTOC);
	fFPContractions->SetValue(fNewSettings.pFPContractions);
	fTraceback->SetValue(fNewSettings.pEmitTraceback);
	fProfile->SetValue(fNewSettings.pEmitProfile);

	// Instruction Scheduling Popup
	item = fInstructionScheduling->ItemAt(fNewSettings.pInstructionScheduling);
	ASSERT(item);
	if (item && item != fInstructionScheduling->FindMarked())
		item->SetMarked(true);

	// Optimize for Speed/Size Popup
	item = fSpeedSizePopup->ItemAt(fNewSettings.pOptimizeForSpeed ? 0 : 1);
	ASSERT(item);
	if (item && item != fSpeedSizePopup->FindMarked())
		item->SetMarked(true);

	// Peephole optimization
	fPeephole->SetValue(fNewSettings.pPeepholeOn);

	// Global Optimization Popup
	item = fOptimizationLevelPopup->ItemAt(fNewSettings.pOptimizationLevel);
	ASSERT(item);
	if (item && item != fOptimizationLevelPopup->FindMarked())
	{
		item->SetMarked(true);
		SetInfoText();
	}
}

// ---------------------------------------------------------------------------
//		 SetInfoText
// ---------------------------------------------------------------------------

void
MProcessorView::SetInfoText()
{
	switch (fNewSettings.pOptimizationLevel)
	{
		case 0:
			fInfoView->SetText(kNoOptText);
			break;
		case 1:
			fInfoView->SetText(kOptOneText);
			break;
		case 2:
			fInfoView->SetText(kOptTwoText);
			break;
		case 3:
			fInfoView->SetText(kOptThreeText);
			break;
		case 4:
			fInfoView->SetText(kOptFourText);
			break;
	}
}

// ---------------------------------------------------------------------------
//		 DoFactorySettings
// ---------------------------------------------------------------------------

void
MProcessorView::DoFactorySettings()
{
	MDefaultPrefs::SetProcessorDefaults(fNewSettings);

	UpdateValues();
//	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MProcessorView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		 Title
// ---------------------------------------------------------------------------

const char *
MProcessorView::Title()
{
	return titleProc;
}

// ---------------------------------------------------------------------------
//		 Targets
// ---------------------------------------------------------------------------

TargetT
MProcessorView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		 ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MProcessorView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kCompileUpdate;
}

// ---------------------------------------------------------------------------
//		 DoSave
// ---------------------------------------------------------------------------

void
MProcessorView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		 DoRevert
// ---------------------------------------------------------------------------

void
MProcessorView::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		 FilterKeyDown
// ---------------------------------------------------------------------------

bool
MProcessorView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		 ValueChanged
// ---------------------------------------------------------------------------

void
MProcessorView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		 AttachedToWindow
// ---------------------------------------------------------------------------

void
MProcessorView::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BCheckBox*		checkBox;
	BMessage*		msg;
	BPopUpMenu*		popup;
	MTextView*		textView;
	BMenuField*		menufield;
	float			top = 15.0;
	const float		left = 10.0;

	// Box
	r = Bounds();
	box = new BBox(r, "processor");
	box->SetLabel("Processor Settings");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Make Strings Read Only
	r.Set(10, top, 160, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "stringsro", "Make Strings Read Only", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fStringsROnly = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Store Static Data in TOC
	r.Set(10, top, 160, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "datainTOC", "Store Static Data in TOC", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fStaticInTOC = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Use FMADD and FMSUB
	r.Set(10, top, 160, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "fpcontractions", "Use FMADD and FMSUB", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fFPContractions = checkBox;

	// Generate Profiler info
	top = 15.0;
	r.Set(180, top, 360, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "profile", "Generate Profiler Information", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fProfile = checkBox;
	top += 20.0;

	// Emit Traceback Tables
	r.Set(180, top, 320, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "traceback", "Emit Traceback Tables", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fTraceback = checkBox;
	top += 45.0;

	// Optimizations
	// Peephole Optimization
	r.Set(10, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "peep", "Peephole Optimization", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fPeephole = checkBox;
	top += 20.0;

	// Instruction Scheduling popup
	r.Set(left, top, left + 250, top + 18.0);
	popup = new BPopUpMenu("scheduling");
	fInstructionScheduling = popup;

	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Off", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("601", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("603", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("604", msg));

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "instrbar", "Instruction Scheduling:", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	const float		divider = menufield->StringWidth("Global Optimization Level:") + 10.0;

	menufield->SetDivider(divider);
	SetGrey(menufield, kLtGray);
	top += 28;

	// Optimize for Speed/Size popup
	r.Set(left, top, left + 250, top + 18.0);

	popup = new BPopUpMenu("optimizefor");
	fSpeedSizePopup = popup;

	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Speed", msg));
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Space", msg));

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "optbar", "Optimize For:", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	
	//menufield->SetFontName(kChicago);
	menufield->SetDivider(divider);
	SetGrey(menufield, kLtGray);
	top += 28;

	// Global Optimization popup
	r.Set(left, top, left + 250, top + 18.0);

	popup = new BPopUpMenu("optimizefor");
	fOptimizationLevelPopup = popup;
	msg = new BMessage(msgPopupChanged);
	popup->AddItem(new BMenuItem("Off", msg));

	String	level;

	for (long i = 1; i <= 4; i++)
	{
		level = i;

		msg = new BMessage(msgPopupChanged);
		popup->AddItem(new BMenuItem(level, msg));
	}

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "optlevlbar", "Global Optimization Level:", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(divider);
	SetGrey(menufield, kLtGray);
	top += 28;

	// Info on global optimizations
	r.Set(10, top, 325, top + 38);
	textView = new MTextView(r, "PrefixName"); 
	box->AddChild(textView);
	SetGrey(textView, kLtGray);

	textView->MakeSelectable(false);
	textView->MakeEditable(false);
	textView->SetWordWrap(true);
	fInfoView = textView;

	UpdateCheckBoxes();
}
