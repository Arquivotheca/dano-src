//========================================================================
//	MDisassembleView.cpp
//	Copyright 1995 Metrowerks Corporation, All Rights Reserved.
//========================================================================	
//	BDS

#include <string.h>

#include "MDisassembleView.h"
#include "MDefaultPrefs.h"
#include "Utils.h"
#include "IDEMessages.h"
#include "MPlugin.h"

#include <CheckBox.h>
#include <Box.h>
#include <Debug.h>
#include <Window.h>

const char * titleDisasm = "Code Generation/PPC Disassembler";

// ---------------------------------------------------------------------------
//		 MDisassembleView
// ---------------------------------------------------------------------------
//	Constructor

MDisassembleView::MDisassembleView(
	BRect				inFrame)
	: MPlugInPrefsView(inFrame, "warningsview", B_FOLLOW_ALL_SIDES, B_WILL_DRAW)
{
//	SetPointers(&fOldSettings, &fNewSettings, sizeof(fNewSettings));
}

// ---------------------------------------------------------------------------
//		 ~MDisassembleView
// ---------------------------------------------------------------------------
//	Destructor

MDisassembleView::~MDisassembleView()
{
}

// ---------------------------------------------------------------------------
//		 MessageReceived
// ---------------------------------------------------------------------------

void
MDisassembleView::MessageReceived(
	BMessage * 	inMessage)
{
	switch (inMessage->what)
	{
		case msgCheckBoxChanged:
			UpdateCheckBoxes();
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
MDisassembleView::UpdateCheckBoxes()
{
	fNewSettings.pShowCode = fShowCodeCB->Value() == 1;
	fNewSettings.pUseExtended = fUseExtendedCB->Value() == 1;
	fNewSettings.pShowSource = fShowSourceCB->Value() == 1;
	fNewSettings.pOnlyOperands = fOnlyOperandsCB->Value() == 1;

	fUseExtendedCB->SetEnabled(fNewSettings.pShowCode);
	fShowSourceCB->SetEnabled(fNewSettings.pShowCode);
	fOnlyOperandsCB->SetEnabled(fNewSettings.pShowCode);

	fNewSettings.pShowData = fShowDataCB->Value() == 1;
	fNewSettings.pExceptionTable = fExceptionTableCB->Value() == 1;

	fExceptionTableCB->SetEnabled(fNewSettings.pShowData);

	fNewSettings.pShowSYM = fShowSYMCB->Value() == 1;
	fNewSettings.pShowNameTable = fShowNameTableCB->Value() == 1;
	
	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 GetData
// ---------------------------------------------------------------------------
//	Fill the BMessage with preferences data of the kind that this preferences
//	view knows about.

void
MDisassembleView::GetData(
	BMessage&	inOutMessage)
{
	fNewSettings.SwapHostToBig();
	inOutMessage.AddData(kDisAsmPrefs, kMWCCPlugType, &fNewSettings, sizeof(fNewSettings));
	fNewSettings.SwapBigToHost();
}

// ---------------------------------------------------------------------------
//		 SetData
// ---------------------------------------------------------------------------
//	Extract preferences data of the kind that this preferences
//	view knows about from the BMessage, and set the fields in the view.

void
MDisassembleView::SetData(
	BMessage&	inMessage)
{
	DisassemblePrefs*		prefs;
	long			length;
	
	if (B_NO_ERROR == inMessage.FindData(kDisAsmPrefs, kMWCCPlugType, (const void**) &prefs, &length))
	{
		ASSERT(length == sizeof(DisassemblePrefs));

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
MDisassembleView::UpdateValues()
{
	// CheckBoxes
	fShowCodeCB->SetValue(fNewSettings.pShowCode);
	fUseExtendedCB->SetValue(fNewSettings.pUseExtended);
	fShowSourceCB->SetValue(fNewSettings.pShowSource);
	fOnlyOperandsCB->SetValue(fNewSettings.pOnlyOperands);

	fUseExtendedCB->SetEnabled(fNewSettings.pShowCode);
	fShowSourceCB->SetEnabled(fNewSettings.pShowCode);
	fOnlyOperandsCB->SetEnabled(fNewSettings.pShowCode);

	fShowDataCB->SetValue(fNewSettings.pShowData);
	fExceptionTableCB->SetValue(fNewSettings.pExceptionTable);

	fExceptionTableCB->SetEnabled(fNewSettings.pShowData);

	fShowSYMCB->SetValue(fNewSettings.pShowSYM);
	fShowNameTableCB->SetValue(fNewSettings.pShowNameTable);
}

// ---------------------------------------------------------------------------
//		 GetPointers
// ---------------------------------------------------------------------------
//	Provide the addresses and length of the new and old structs that hold
//	the data for this view.  If the data isn't held in a simple struct
//	then don't return any values.  If the values are returned then 
//	Revert will be handled automatically.

void
MDisassembleView::GetPointers(
	void*&	outOld,
	void*&	outNew,
	long&	outLength)
{
	outOld = &fOldSettings;
	outNew = &fNewSettings;
	outLength = sizeof(fOldSettings);
}

// ---------------------------------------------------------------------------
//		 DoFactorySettings
// ---------------------------------------------------------------------------

void
MDisassembleView::DoFactorySettings()
{
	MDefaultPrefs::SetDisassemblerDefaults(fNewSettings);

	UpdateValues();
//	ValueChanged();
}

// ---------------------------------------------------------------------------
//		 Title
// ---------------------------------------------------------------------------

const char *
MDisassembleView::Title()
{
	return titleDisasm;
}

// ---------------------------------------------------------------------------
//		 Targets
// ---------------------------------------------------------------------------

TargetT
MDisassembleView::Targets()
{
	return (kMWDefaults | kCurrentProject);
}

// ---------------------------------------------------------------------------
//		 DoSave
// ---------------------------------------------------------------------------

void
MDisassembleView::DoSave()
{
}

// ---------------------------------------------------------------------------
//		 DoRevert
// ---------------------------------------------------------------------------

void
MDisassembleView::DoRevert()
{
}

// ---------------------------------------------------------------------------
//		 ValueChanged
// ---------------------------------------------------------------------------

void
MDisassembleView::ValueChanged()
{
	Window()->PostMessage(msgPluginViewModified);
}

// ---------------------------------------------------------------------------
//		 FilterKeyDown
// ---------------------------------------------------------------------------

bool
MDisassembleView::FilterKeyDown(
	ulong	/*aKey*/)
{
	return true;
}

// ---------------------------------------------------------------------------
//		 ProjectRequiresUpdate
// ---------------------------------------------------------------------------

bool
MDisassembleView::ProjectRequiresUpdate(
	UpdateType /*inType*/)
{
	return false;
}

// ---------------------------------------------------------------------------
//		 AttachedToWindow
// ---------------------------------------------------------------------------

void
MDisassembleView::AttachedToWindow()
{
	BRect			r;
	BBox*			box;
	BMessage*		msg;
	BCheckBox*		checkBox;
	float			top	= 15;
	const float		left1 = 10;
	const float		left2 = 25;

	// Box
	r = Bounds();
	box = new BBox(r, "AppInfo");
	box->SetLabel("PPC Disassembler Options");
	AddChild(box);
	box->SetFont(be_bold_font);
//	box->SetFontName(kChicago);
	SetGrey(box, kLtGray);

	// Show Code Modules
	r.Set(left1, top, left1 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "showcode", "Show Code Modules", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fShowCodeCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Use Extended Mnemonics
	r.Set(left2, top, left2 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "", "Use Extended Mnemonics", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fUseExtendedCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Show Source Code
	r.Set(left2, top, left2 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "", "Show Source Code", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fShowSourceCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Only Show Operands and Mnemonics
	r.Set(left2, top, left2 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "", "Show Only Operands and Mnemonics", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fOnlyOperandsCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 25.0;

	// Show Data Modules
	r.Set(left1, top, left1 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "", "Show Data Modules", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fShowDataCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 20.0;

	// Disassemble Exception Tables
	r.Set(left2, top, left2 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "", "Disassemble Exception Tables", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fExceptionTableCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 25.0;

	// Show SYM Info
	r.Set(left1, top, left1 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "", "Show SYM Info", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fShowSYMCB = checkBox;
	SetGrey(checkBox, kLtGray);
	top += 25.0;

	// Show Name Table
	r.Set(left1, top, left1 + 280, top + 15);
	msg = new BMessage(msgCheckBoxChanged);
	checkBox = new BCheckBox(r, "", "Show Name Table", msg); 
	box->AddChild(checkBox);
	checkBox->SetTarget(this);
	fShowNameTableCB = checkBox;
	SetGrey(checkBox, kLtGray);

	UpdateCheckBoxes();
}
