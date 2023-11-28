//========================================================================
//	MLinkerView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MLinkerView.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MTextControl.h"
#include "Utils.h"
#include "IDEMessages.h"
#include "MPlugin.h"

#include <Box.h>
#include <CheckBox.h>
#include <Debug.h>
#include <Window.h>

const char * titleLink = "Linker/PPC Linker";

// ---------------------------------------------------------------------------
//		 MLinkerView
// ---------------------------------------------------------------------------
//	Constructor

MLinkerView::MLinkerView(
	BRect		inFrame)
	: MPlugInPrefsView(inFrame, "linkerview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		 ~MLinkerView
// ---------------------------------------------------------------------------
//	Destructor

MLinkerView::~MLinkerView()
{
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MLinkerView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgCheckBoxChanged:
			UpdateCheckBoxes();
			break;

		case msgTextChanged:
			UpdateTextBoxes();
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
MLinkerView::UpdateCheckBoxes()
{
	fNewSettings.pGenerateSYMFile = fGenSymCB->Value() == 1;
	fNewSettings.pUseFullPath = fFulPathCB->Value() == 1;
	fNewSettings.pGenerateLinkMap = fLinkMapCB->Value() == 1;

	fFulPathCB->SetEnabled(fNewSettings.pGenerateSYMFile);

	fNewSettings.pSuppressWarnings = fSuppressWarningsCB->Value() == 1;
	fNewSettings.pDeadStrip = fDeadSTripCB->Value() == 1;
	
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 UpdateTextBoxes
// ---------------------------------------------------------------------------

void
MLinkerView::UpdateTextBoxes()
{
	strcpy(fNewSettings.pInit, fInitBox->Text());
	strcpy(fNewSettings.pMain, fMainBox->Text());
	strcpy(fNewSettings.pTerm, fTermBox->Text());

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MLinkerView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();
	inOutMessage.AddData(kLinkerPrefs, kMWLDPlugType, &fNewSettings, sizeof(fNewSettings), false);
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		 SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MLinkerView::SetData(
	BMessage&	inMessage)
{
	ssize_t			length;
	LinkerPrefs*	prefs;

	if (B_NO_ERROR == inMessage.FindData(kLinkerPrefs, kMWLDPlugType, (const void**) &prefs, &length))
	{
		ASSERT(length == sizeof(LinkerPrefs));

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
MLinkerView::UpdateValues()
{
	// CheckBoxes
	fGenSymCB->SetValue(fNewSettings.pGenerateSYMFile);
	fFulPathCB->SetValue(fNewSettings.pUseFullPath);
	fFulPathCB->SetEnabled(fNewSettings.pGenerateSYMFile);
	fLinkMapCB->SetValue(fNewSettings.pGenerateLinkMap);
	fSuppressWarningsCB->SetValue(fNewSettings.pSuppressWarnings);
	fDeadSTripCB->SetValue(fNewSettings.pDeadStrip);
	
	// TextBoxes
	fInitBox->SetText(fNewSettings.pInit);
	fTermBox->SetText(fNewSettings.pTerm);
	fMainBox->SetText(fNewSettings.pMain);
}

// ---------------------------------------------------------------------------
//		 DoFactorySettings
// ---------------------------------------------------------------------------

void
MLinkerView::DoFactorySettings()
{
	MDefaultPrefs::SetLinkerDefaults(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MLinkerView::GetPointers(
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
MLinkerView::Title()
{
	return titleLink;
}

// ---------------------------------------------------------------------------
//		 Targets
// ---------------------------------------------------------------------------

TargetT
MLinkerView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		 DoSave
// ---------------------------------------------------------------------------

void
MLinkerView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		 DoRevert
// ---------------------------------------------------------------------------

void
MLinkerView::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		 FilterKeyDown
// ---------------------------------------------------------------------------

bool
MLinkerView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		 ValueChanged
// ---------------------------------------------------------------------------

void
MLinkerView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		 ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MLinkerView::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kLinkUpdate;
}

// ---------------------------------------------------------------------------
//		 AttachedToWindow
// ---------------------------------------------------------------------------

void
MLinkerView::AttachedToWindow()
{
	BRect			r;
	BRect			rr;
	BBox*			box;
	BMessage*		msg;
	BCheckBox*		checkBox;
	BStringView*	caption;
	BTextView*		text;
	MTextControl*	textControl;
	float			top	= 15.0;

	// Box
	r = Bounds();
	box = new BBox(r, "AppInfo");
	box->SetLabel("Link Options");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Generate SYM File
	r.Set(10, top, 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "GenSYM", "Generate SYM File", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fGenSymCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Use full path names
	r.Set(25, top, 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Full Path", "Use Full Path Names", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fFulPathCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 25.0;

	// Generate Link Map
	r.Set(10, top, 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Link Map", "Generate Link Map", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fLinkMapCB = checkBox;
	top += 20.0;

	// Suppress Warning Messages
	r.Set(10, top, 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Link Map", "Suppress Warning Messages", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fSuppressWarningsCB = checkBox;
	top += 20.0;

	// Dead-strip Unused Static Initialization Code
	r.Set(10, top, 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Link Map", "Dead-strip Unused Static Initialization Code", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fDeadSTripCB = checkBox;

	// TextBoxes
	// Entry Points:
	top += 30.0;
	r.Set(10, top, 90, top + 16.0);
	caption = new BStringView(r, "st1", "Entry Points:"); 
	box->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);
	top += 20.0;

	// Initialization:
	r.Set(20, top, 260, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st1", "Initialization:", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(80);
	SetGrey(textControl, kLtGray);
	
	textControl->SetTarget(this);

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(64);
	DisallowInvalidChars(*text);
	fInitBox = text;
	top += 25.0;

	// Main:
	r.Set(20, top, 260, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st2", "Main:", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(80);
	SetGrey(textControl, kLtGray);
	
	textControl->SetTarget(this);

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(64);
	DisallowInvalidChars(*text);
	fMainBox = text;
	top += 25.0;

	// Termination:
	r.Set(20, top, 260, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st3", "Termination:", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(80);
	SetGrey(textControl, kLtGray);
	
	textControl->SetTarget(this);

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(64);
	DisallowInvalidChars(*text);
	fTermBox = text;
	top += 25.0;

	UpdateCheckBoxes();
}
