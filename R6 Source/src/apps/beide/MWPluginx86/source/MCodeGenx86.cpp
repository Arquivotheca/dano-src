//========================================================================
//	MCodeGenx86.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include "MCodeGenx86.h"
#include "MDefaultPrefs.h"
#include "MTextView.h"
#include "Utils.h"
#include "IDEMessages.h"
#include "CString.h"

const char * titleCodGen = "Code Generation/x86 Code Gen";

// ---------------------------------------------------------------------------
//		¥ MCodeGenx86
// ---------------------------------------------------------------------------
//	Constructor

MCodeGenx86::MCodeGenx86(
	BRect				inFrame)
	: MPlugInPrefsView(inFrame, "x86codegen", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
}

// ---------------------------------------------------------------------------
//		¥ ~MCodeGenx86
// ---------------------------------------------------------------------------
//	Destructor

MCodeGenx86::~MCodeGenx86()
{
}

// ---------------------------------------------------------------------------
//		¥ MessageReceived
// ---------------------------------------------------------------------------

void
MCodeGenx86::MessageReceived(
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
//		¥ UpdateCheckBoxes
// ---------------------------------------------------------------------------

void
MCodeGenx86::UpdateCheckBoxes()
{
	fNewSettings.pPeepholeOn = fPeepCB->Value() == 1;
	fNewSettings.pShowMachineCodeListing = fMachineCodeCB->Value() == 1;
	fNewSettings.pUseRegisterColoring = fRegColoringCB->Value() == 1;
	fNewSettings.pInlineIntrinsics = fInlineIntrinsicsCB->Value() == 1;
#if 0
	fNewSettings.pGenerateSYM = fTraceback->Value() == 1;
	fNewSettings.pGenerateCodeView = fProfile->Value() == 1;
#endif

	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ UpdatePopups
// ---------------------------------------------------------------------------

void
MCodeGenx86::UpdatePopups()
{
#if 0
	BMenuItem*		item;
	
	// Byte Alignment
	item = fAlignmentPopup->FindMarked();
	ASSERT(item);
	if (item)
		fNewSettings.pAlignmentKind = (uchar) fAlignmentPopup->IndexOf(item);

	ValueChanged();
#endif
}

// ---------------------------------------------------------------------------
//		¥ GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MCodeGenx86::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToLittle();
	inOutMessage.AddData(kCodeGenPrefsx86, kMWCCx86Type, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapLittleToHost();
}

// ---------------------------------------------------------------------------
//		¥ SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MCodeGenx86::SetData(
	BMessage&	inMessage)
{
	CodeGenx86*			prefs;
	long				length;

	if (B_NO_ERROR == inMessage.FindData(kCodeGenPrefsx86, kMWCCx86Type, &prefs, &length))
	{
		ASSERT(length == sizeof(CodeGenx86));

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
MCodeGenx86::UpdateValues()
{
	// CheckBoxes
	fPeepCB->SetValue(fNewSettings.pPeepholeOn);
	fMachineCodeCB->SetValue(fNewSettings.pShowMachineCodeListing);
	fRegColoringCB->SetValue(fNewSettings.pUseRegisterColoring);
	fInlineIntrinsicsCB->SetValue(fNewSettings.pInlineIntrinsics);

#if 0
	BMenuItem*		item;

	// Byte Alignment Popup
	item = fAlignmentPopup->ItemAt(fNewSettings.pAlignmentKind);
	ASSERT(item);
	if (item && item != fAlignmentPopup->FindMarked())
		item->SetMarked(true);
#endif
}

// ---------------------------------------------------------------------------
//		¥ DoFactorySettings
// ---------------------------------------------------------------------------

void
MCodeGenx86::DoFactorySettings()
{
	MDefaultPrefs::SetCodeGenx86Defaults(fNewSettings);

	UpdateValues();
//	ValueChanged();
}

// ---------------------------------------------------------------------------
//		¥ GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MCodeGenx86::GetPointers(
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
MCodeGenx86::Title()
{
	return titleCodGen;
}

// ---------------------------------------------------------------------------
//		¥ Targets
// ---------------------------------------------------------------------------

TargetT
MCodeGenx86::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		¥ ProjectRequiresUpdate
// ---------------------------------------------------------------------------
//	A change in this prefs panel requires that the project be relinked or
//	recompiled.

bool
MCodeGenx86::ProjectRequiresUpdate(
	UpdateType inType)
{
	return inType == kCompileUpdate;
}

// ---------------------------------------------------------------------------
//		¥ DoSave
// ---------------------------------------------------------------------------

void
MCodeGenx86::DoSave()
{
}

// ---------------------------------------------------------------------------
//		¥ DoRevert
// ---------------------------------------------------------------------------

void
MCodeGenx86::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		¥ FilterKeyDown
// ---------------------------------------------------------------------------

bool
MCodeGenx86::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		¥ ValueChanged
// ---------------------------------------------------------------------------

void
MCodeGenx86::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		¥ AttachedToWindow
// ---------------------------------------------------------------------------

void
MCodeGenx86::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BCheckBox*		checkBox;
	BMessage*		msg;
//	BPopUpMenu*		popup;
//	BMenuField*		menufield;
	float			top = 15.0;
	const float		left = 10.0;

	// Box
	r = Bounds();
	box = new BBox(r, "processor");
	box->SetLabel("x86 Code Generation");
	AddChild(box);
	box->SetFont(be_bold_font);
	SetGrey(box, kLtGray);

	// Byte Alignment popup
	// do we need this?
#if 0
	r.Set(left, top, 350, top + 16);

	popup = new BPopUpMenu("align");
	fAlignmentPopup = popup;

	const char*		alignsizes[] = { "1", "2", "4", "8", "16" };
	const int32		numAlignments = 5;

	for (int i = 0; i < numAlignments; i++)
	{
		msg = new BMessage(msgPopupChanged);
		popup->AddItem(new BMenuItem(alignsizes[i], msg));
	}

	popup->SetTargetForItems(this);

	menufield = new BMenuField(r, "projtype", "Byte Alignment", popup);
	box->AddChild(menufield);
	menufield->SetFont(be_bold_font);
	menufield->SetDivider(130);
	SetGrey(menufield, kLtGray);
	top += 30.0;
#endif

	// Peephole Optimizer
	r.Set(10, top, 180, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb1", "Peephole Optimizer", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fPeepCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Machine Code Listing
	r.Set(10, top, 180, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb2", "Machine Code Listing", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fMachineCodeCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Register Coloring
	r.Set(10, top, 180, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb3", "Register Coloring", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fRegColoringCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Inline Intrinsic Functions
	r.Set(10, top, 180, top + 15.0);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "cb4", "Inline Intrinsic Functions", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fInlineIntrinsicsCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	UpdateCheckBoxes();
}