//========================================================================
//	MLinkerViewx86.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MLinkerViewx86.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "MTextControl.h"
#include "Utils.h"
#include "IDEMessages.h"

const char * titleLink = "Linker/x86 Linker";

// ---------------------------------------------------------------------------
//		¥ MLinkerViewx86
// ---------------------------------------------------------------------------
//	Constructor

MLinkerViewx86::MLinkerViewx86(
	BRect		inFrame)
	: MPlugInPrefsView(inFrame, "linkerview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		¥ ~MLinkerViewx86
// ---------------------------------------------------------------------------
//	Destructor

MLinkerViewx86::~MLinkerViewx86()
{
}

// ---------------------------------------------------------------------------
//		¥ MessageReceived
// ---------------------------------------------------------------------------

void
MLinkerViewx86::MessageReceived(
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
//		¥ UpdateCheckBoxes
// ---------------------------------------------------------------------------

void
MLinkerViewx86::UpdateCheckBoxes()
{
	fNewSettings.pGenerateSYMFile = fGenSymCB->Value() == 1;
	fNewSettings.pUseFullPath = fFulPathCB->Value() == 1;
//	fNewSettings.pGenerateCVInfo = fGenCVCB->Value() == 1;
	fNewSettings.pGenerateLinkMap = fLinkMapCB->Value() == 1;

	fFulPathCB->SetEnabled(fNewSettings.pGenerateSYMFile);

	fNewSettings.pSuppressWarnings = fSuppressWarningsCB->Value() == 1;
	
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ UpdateTextBoxes
// ---------------------------------------------------------------------------

void
MLinkerViewx86::UpdateTextBoxes()
{
	memset(fNewSettings.pMain, '\0', sizeof(fNewSettings.pMain));
	memset(fNewSettings.pMain, '\0', sizeof(fNewSettings.pCommand));
	strcpy(fNewSettings.pMain, fMainBox->Text());
	strcpy(fNewSettings.pCommand, fCommandFileBox->Text());

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MLinkerViewx86::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToLittle();
	inOutMessage.AddData(kLinkerPrefsx86, kMWLDx86Type, &fNewSettings, sizeof(fNewSettings), false);
	fNewSettings.SwapLittleToHost();
}

// ---------------------------------------------------------------------------
//		¥ SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MLinkerViewx86::SetData(
	BMessage&	inMessage)
{
	ssize_t			length;
	LinkerPrefsx86*	prefs;

	if (B_NO_ERROR == inMessage.FindData(kLinkerPrefsx86, kMWLDx86Type, &prefs, &length))
	{
		ASSERT(length == sizeof(LinkerPrefsx86));

		fNewSettings = *prefs;
		fNewSettings.SwapLittleToHost();
		fOldSettings = fNewSettings;
		
		UpdateValues();
	}
}

// ---------------------------------------------------------------------------
//		¥ UpdateValues
// ---------------------------------------------------------------------------
//	Read the values from the NewSettings struct and set the controls 
//	accordingly.

void
MLinkerViewx86::UpdateValues()
{
	// CheckBoxes
	fGenSymCB->SetValue(fNewSettings.pGenerateSYMFile);
	fFulPathCB->SetValue(fNewSettings.pUseFullPath);
	fFulPathCB->SetEnabled(fNewSettings.pGenerateSYMFile);
	fLinkMapCB->SetValue(fNewSettings.pGenerateLinkMap);
	fSuppressWarningsCB->SetValue(fNewSettings.pSuppressWarnings);
//	fGenCVCB->SetValue(fNewSettings.pGenerateCVInfo);
	
	// TextBoxes
	fMainBox->SetText(fNewSettings.pMain);
	fCommandFileBox->SetText(fNewSettings.pCommand);
}

// ---------------------------------------------------------------------------
//		¥ DoFactorySettings
// ---------------------------------------------------------------------------

void
MLinkerViewx86::DoFactorySettings()
{
	MDefaultPrefs::SetLinkerDefaultsx86(fNewSettings);

	UpdateValues();
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MLinkerViewx86::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		¥ Title
// ---------------------------------------------------------------------------

const char *
MLinkerViewx86::Title()
{
	return titleLink;
}

// ---------------------------------------------------------------------------
//		¥ Targets
// ---------------------------------------------------------------------------

TargetT
MLinkerViewx86::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		¥ DoSave
// ---------------------------------------------------------------------------

void
MLinkerViewx86::DoSave()
{
}

// ---------------------------------------------------------------------------
//		¥ DoRevert
// ---------------------------------------------------------------------------

void
MLinkerViewx86::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		¥ FilterKeyDown
// ---------------------------------------------------------------------------

bool
MLinkerViewx86::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		¥ ValueChanged
// ---------------------------------------------------------------------------

void
MLinkerViewx86::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		¥ ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MLinkerViewx86::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kLinkUpdate;
}

// ---------------------------------------------------------------------------
//		¥ AttachedToWindow
// ---------------------------------------------------------------------------

void
MLinkerViewx86::AttachedToWindow()
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

#if 0
	// Generate CodeView Info
	r.Set(10, top, 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "Link Map", "Generate CodeView Info", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fGenCVCB = checkBox;
#endif

	// TextBoxes
	// Entry Point:
	top += 30.0;
	r.Set(10, top, 90, top + 16.0);
	caption = new BStringView(r, "st1", "Entry Point:"); 
	box->AddChild(caption);
	caption->SetFont(be_bold_font);
	SetGrey(caption, kLtGray);
	top += 20.0;

#if 0
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
#endif

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

#if 0
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
#endif

	top += 10.0;
	// Command File:
	r.Set(10, top, 260, top + 14.0);
	msg = new BMessage(msgTextChanged);
	textControl = new MTextControl(r, "st3", "Command File:", "", msg);
	box->AddChild(textControl);
	textControl->SetFont(be_bold_font);
	textControl->SetDivider(90);
	SetGrey(textControl, kLtGray);
	
	textControl->SetTarget(this);

	text = (BTextView*) textControl->ChildAt(0);
	text->SetMaxBytes(64);
	DisallowInvalidChars(*text);
	fCommandFileBox = text;
	top += 25.0;

	UpdateCheckBoxes();
}
