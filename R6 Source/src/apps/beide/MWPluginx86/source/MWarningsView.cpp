//========================================================================
//	MWarningsView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MWarningsView.h"
#include "MDefaultPrefs.h"
#include "Utils.h"
#include "IDEMessages.h"

const char * titleWarn = "Language/C/C++ Warnings";

// ---------------------------------------------------------------------------
//		¥ MWarningsView
// ---------------------------------------------------------------------------
//	Constructor

MWarningsView::MWarningsView(
	BRect				inFrame)
	: MPlugInPrefsView(inFrame, "warningsview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		¥ ~MWarningsView
// ---------------------------------------------------------------------------
//	Destructor

MWarningsView::~MWarningsView()
{
}

// ---------------------------------------------------------------------------
//		¥ MessageReceived
// ---------------------------------------------------------------------------

void
MWarningsView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case  msgCheckBoxChanged:
			UpdateCheckBoxes();
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
MWarningsView::UpdateCheckBoxes()
{
	fNewSettings.pWarningsAreErrors = fWarningsAreErrors->Value() == 1;
	fNewSettings.pIllegalPragmas = fIllegalPragmas->Value() == 1;
	fNewSettings.pEmptyDeclarations = fEmptyDeclarations->Value() == 1;
	fNewSettings.pPossibleErrors = fPossibleErrors->Value() == 1;
	fNewSettings.pUnusedVariables = fUnusedVariables->Value() == 1;
	fNewSettings.pUnusedArguments = fUnusedArguments->Value() == 1;
	fNewSettings.pExtraCommas = fExtraCommas->Value() == 1;
	fNewSettings.pExtendedErrChecking = fExtendedChecking->Value() == 1;
	fNewSettings.pHiddenVirtuals = fHiddenVirtuals->Value() == 1;
	fNewSettings.pLargeArgs = fLargeArgs->Value() == 1;

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MWarningsView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapFromHost();
	inOutMessage.AddData(kWarningPrefsType, kCompilerType, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapToHost();
}

// ---------------------------------------------------------------------------
//		¥ SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MWarningsView::SetData(
	BMessage&	inMessage)
{
	WarningsPrefs*		prefs;
	long				length;

	if (B_NO_ERROR == inMessage.FindData(kWarningPrefsType, kCompilerType, &prefs, &length))
	{
		ASSERT(length == sizeof(WarningsPrefs));

		fNewSettings = *prefs;
		fNewSettings.SwapToHost();
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
MWarningsView::UpdateValues()
{
	// CheckBoxes
	fWarningsAreErrors->SetValue(fNewSettings.pWarningsAreErrors);
	fIllegalPragmas->SetValue(fNewSettings.pIllegalPragmas);
	fEmptyDeclarations->SetValue(fNewSettings.pEmptyDeclarations);
	fPossibleErrors->SetValue(fNewSettings.pPossibleErrors);
	fUnusedVariables->SetValue(fNewSettings.pUnusedVariables);
	fUnusedArguments->SetValue(fNewSettings.pUnusedArguments);
	fExtraCommas->SetValue(fNewSettings.pExtraCommas);
	fExtendedChecking->SetValue(fNewSettings.pExtendedErrChecking);
	fHiddenVirtuals->SetValue(fNewSettings.pHiddenVirtuals);
	fLargeArgs->SetValue(fNewSettings.pLargeArgs);
}

// ---------------------------------------------------------------------------
//		¥ DoFactorySettings
// ---------------------------------------------------------------------------

void
MWarningsView::DoFactorySettings()
{
	MDefaultPrefs::SetWarningsDefaults(fNewSettings);

	UpdateValues();
//	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ Title
// ---------------------------------------------------------------------------

const char *
MWarningsView::Title()
{
	return titleWarn;
}

// ---------------------------------------------------------------------------
//		¥ Targets
// ---------------------------------------------------------------------------

TargetT
MWarningsView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		¥ GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MWarningsView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		¥ AttachedToWindow
// ---------------------------------------------------------------------------

void
MWarningsView::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BCheckBox*		checkBox;
	BMessage*		msg;
	float			top = 15.0;

	// Box
	r = Bounds();
	box = new BBox(r, "Warnings");
	box->SetLabel("Warnings Info");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Treat All Warnings as Errors
	r.Set(10, top, 200, 25);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "WarnError", "Treat All Warnings as Errors", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fWarningsAreErrors = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Illegal Pragmas
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "illegalpragmas", "Illegal Pragmas", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fIllegalPragmas = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Empty Declarations
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "edecl", "Empty Declarations", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fEmptyDeclarations = checkBox;
	top += 20.0;

	// Possible Errors
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "posserrors", "Possible Errors", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fPossibleErrors = checkBox;
	top += 20.0;

	// Unused Variables
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "unused", "Unused Variables", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fUnusedVariables = checkBox;
	top += 20.0;

	// Unused Arguments
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "unusedargs", "Unused Arguments", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fUnusedArguments = checkBox;
	top += 20.0;

	// Extra Commas
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "edecl", "Extra Commas", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fExtraCommas = checkBox;
	top += 20.0;

	// Extended Error Checking
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "extra", "Extended Error Checking", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fExtendedChecking = checkBox;
	top += 20.0;

	// Hidden virtual functions
	r.Set(30, top, 200, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "extra", "Hidden Virtual Functions", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fHiddenVirtuals = checkBox;
	top += 20.0;

	// Large args
	r.Set(30, top, 300, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "extra", "Large args passed to unprototyped functions", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	SetGrey(checkBox, kLtGray);
	fLargeArgs = checkBox;

	UpdateCheckBoxes();
}

// ---------------------------------------------------------------------------
//		¥ DoSave
// ---------------------------------------------------------------------------

void
MWarningsView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		¥ DoRevert
// ---------------------------------------------------------------------------

void
MWarningsView::DoRevert()
{
}
// ---------------------------------------------------------------------------
//		¥ FilterKeyDown
// ---------------------------------------------------------------------------

bool
MWarningsView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		¥ ProjectRequiresUpdate
// ---------------------------------------------------------------------------

bool
MWarningsView::ProjectRequiresUpdate(
	UpdateType /*inType*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		¥ ValueChanged
// ---------------------------------------------------------------------------

void
MWarningsView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}